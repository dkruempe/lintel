#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

namespace Hypodermic {

    /**
     * Minimal replacement for the subset of boost::signals2::signal that
     * Hypodermic relies on: void-returning slots, connect(), disconnect() and
     * disconnect_all_slots() plus the call operator used for emitting.
     *
     * Dropped in favour of <boost/signals2.hpp> because that single header
     * accounted for roughly 109k preprocessed lines and was the heaviest
     * include of the whole DI container, while the container only ever needs
     * the three operations implemented below.
     *
     * Semantics kept identical to boost::signals2's default configuration:
     *  - slot access is guarded by a mutex, so connect()/emit() are safe to
     *    interleave across threads,
     *  - the set of slots to invoke is snapshotted before the first slot runs,
     *    and each slot is re-checked before being called, so a slot may
     *    connect() or disconnect_all_slots() while the signal is being
     *    emitted. ContainerBuilder relies on this when it tears down its own
     *    slot from within that slot.
     *  - signals are neither copyable nor movable, matching signals2.
     */
    template<typename... TArgs>
    class Signal {
    public:
        typedef std::function<void(TArgs...)> Slot;

        class Connection {
        public:
            Connection() = default;

            Connection(Signal *owner, std::size_t slotId)
                    : m_owner(owner), m_slotId(slotId) {}

            /** Detaches the slot this connection refers to. */
            void disconnect() {
                if (m_owner == nullptr) return;

                m_owner->disconnect(m_slotId);
                m_owner = nullptr;
            }

        private:
            Signal *m_owner = nullptr;
            std::size_t m_slotId = 0;
        };

        Signal() = default;

        Signal(const Signal &) = delete;

        Signal(Signal &&) = delete;

        Signal &operator=(const Signal &) = delete;

        Signal &operator=(Signal &&) = delete;

        /** Appends a slot. Empty slots are ignored, as in signals2. */
        Connection connect(Slot slot) {
            if (!slot) return Connection();

            std::lock_guard<std::mutex> lock(m_mutex);
            const std::size_t slotId = m_nextSlotId++;
            m_slots.push_back(SlotEntry{slotId, std::move(slot)});
            return Connection(this, slotId);
        }

        /** Detaches every slot currently connected. */
        void disconnect_all_slots() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_slots.clear();
        }

        /** Invokes every connected slot in connection order.
         *
         * Slots connected during the emission are not called, slots disconnected
         * during the emission are skipped - matching boost::signals2, which
         * ContainerBuilder relies on when it drops its own slot from within
         * that slot.
         */
        void operator()(TArgs... args) const {
            std::vector<std::size_t> slotIds;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                slotIds.reserve(m_slots.size());
                for (auto &&entry: m_slots) slotIds.push_back(entry.id);
            }

            for (auto slotId: slotIds) {
                Slot slot;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    auto it = findSlot(slotId);
                    if (it == std::end(m_slots)) continue;
                    slot = it->slot;
                }
                slot(args...);
            }
        }

    private:
        struct SlotEntry {
            std::size_t id;
            Slot slot;
        };

        typename std::vector<SlotEntry>::iterator findSlot(std::size_t slotId) {
            for (auto it = std::begin(m_slots); it != std::end(m_slots); ++it) {
                if (it->id == slotId) return it;
            }
            return std::end(m_slots);
        }

        typename std::vector<SlotEntry>::const_iterator findSlot(
                std::size_t slotId) const {
            for (auto it = std::begin(m_slots); it != std::end(m_slots); ++it) {
                if (it->id == slotId) return it;
            }
            return std::end(m_slots);
        }

        void disconnect(std::size_t slotId) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = findSlot(slotId);
            if (it != std::end(m_slots)) m_slots.erase(it);
        }

        mutable std::mutex m_mutex;
        std::vector<SlotEntry> m_slots;
        std::size_t m_nextSlotId = 0;
    };

}  // namespace Hypodermic