#include "lintel/features/base/msg/MessageQueueShmCore.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace lintel {
namespace mqshm_detail {

  constexpr std::size_t kCacheLine = 64;
  /**
   * Per slot header: sequence number, stored byte count and padding.
   *
   * Kept at its natural alignment (16 bytes). Slots are separated by a
   * cache-line-padded stride instead of an `alignas` on the header: the stride
   * already guarantees that consecutive sequence handshakes live on different
   * cache lines, and MSVC warns (C4324, promoted to an error here) when a small
   * struct is padded by an alignment specifier.
   */
  constexpr std::size_t kSlotHeaderSize = 16;
  /** Granularity of the wait used by the blocking operations. */
  constexpr std::chrono::milliseconds kWaitSlice{ 1 };

  [[nodiscard]] std::size_t alignUp(std::size_t value, std::size_t alignment)
  {
    return (value + alignment - 1u) / alignment * alignment;
  }

  /**
   * Fixed-capacity MPMC ring of fixed-size byte slots inside shared memory.
   *
   * The algorithm is the Vyukov bounded MPMC queue used by RingBufferShm and
   * EventBus::ShmRing. The difference is the runtime slot size: every slot is
   * a sequence header followed by `slotSize` payload bytes, and the stride is
   * padded to a cache line so concurrent producers/consumers do not share a
   * cache line for the sequence handshake.
   *
   * The slot array is allocated from the shared segment and addressed through
   * a byte offset relative to this object, exactly like RingBufferShm.
   */
  class ShmRing
  {
  public:
    struct SlotHeader
    {
      std::atomic<std::uint64_t> sequence{ 0 };
      std::uint32_t length{ 0 };
      std::uint32_t padding{ 0 };
    };
    static_assert(sizeof(SlotHeader) == kSlotHeaderSize, "SlotHeader size must match kSlotHeaderSize");

    ShmRing(std::size_t capacity, std::size_t slotSize, ShmSegmentAccessor segment)
      : m_capacity(capacity), m_slotSize(slotSize), m_segment(segment)
    {
      if (capacity == 0) { throw std::invalid_argument("MessageQueueShmCore capacity must be positive"); }
      if (slotSize == 0) { throw std::invalid_argument("MessageQueueShmCore message size must be positive"); }
      if (slotSize > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("MessageQueueShmCore message size is too large");
      }
      if (capacity > std::numeric_limits<std::size_t>::max() / (kSlotHeaderSize + slotSize)) {
        throw std::invalid_argument("MessageQueueShmCore slot layout is too large");
      }

      m_stride = alignUp(kSlotHeaderSize + slotSize, kCacheLine);
      if (capacity > std::numeric_limits<std::size_t>::max() / m_stride) {
        throw std::invalid_argument("MessageQueueShmCore capacity is too large");
      }

      const std::size_t bufferSize = capacity * m_stride;
      const std::size_t allocationSize = bufferSize + kCacheLine - 1u;
      void *raw = segment.allocate(allocationSize);
      if (raw == nullptr) { throw std::bad_alloc(); }
      m_allocation = static_cast<std::byte *>(raw);

      void *alignedRaw = raw;
      std::size_t alignmentSpace = allocationSize;
      if (std::align(kCacheLine, bufferSize, alignedRaw, alignmentSpace) == nullptr) {
        segment.deallocate(raw);
        m_allocation = nullptr;
        throw std::bad_alloc();
      }

      m_bufferOffset = reinterpret_cast<std::intptr_t>(alignedRaw) - reinterpret_cast<std::intptr_t>(this);
      for (std::size_t index = 0; index < capacity; ++index) {
        SlotHeader *slot = slotAt(index);
        new (slot) SlotHeader();
        slot->sequence.store(static_cast<std::uint64_t>(index), std::memory_order_relaxed);
      }
    }

    ~ShmRing()
    {
      if (m_allocation != nullptr) {
        m_segment.deallocate(m_allocation);
        m_allocation = nullptr;
      }
    }

    ShmRing(const ShmRing &) = delete;
    ShmRing &operator=(const ShmRing &) = delete;

    [[nodiscard]] bool push(const void *message, std::size_t size)
    {
      // The sequence number handshake cannot distinguish full from empty when
      // the ring has exactly one slot (both states map to the same sequence),
      // so a single-slot ring uses an explicit state machine instead:
      // 0 = empty, 1 = readable, 2 = currently being written/read.
      if (m_capacity == 1u) {
        std::uint8_t expected = kSingleEmpty;
        if (!m_singleState.compare_exchange_strong(
              expected, kSingleBusy, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return false;
        }
        SlotHeader *slot = slotAt(0);
        slot->length = static_cast<std::uint32_t>(size);
        std::memcpy(payloadOf(slot), message, size);
        m_singleState.store(kSingleReady, std::memory_order_release);
        return true;
      }

      std::uint64_t position = m_head.load(std::memory_order_relaxed);
      for (;;) {
        SlotHeader *slot = slotAt(position);
        const std::uint64_t sequence = slot->sequence.load(std::memory_order_acquire);
        const std::int64_t difference = static_cast<std::int64_t>(sequence - position);
        if (difference == 0) {
          std::uint64_t expected = position;
          if (m_head.compare_exchange_weak(expected, position + 1u, std::memory_order_relaxed)) {
            slot->length = static_cast<std::uint32_t>(size);
            std::memcpy(payloadOf(slot), message, size);
            slot->sequence.store(position + 1u, std::memory_order_release);
            return true;
          }
          position = expected;
        } else if (difference < 0) {
          return false;// full
        } else {
          position = m_head.load(std::memory_order_relaxed);
        }
      }
    }

    [[nodiscard]] bool pop(void *message, std::size_t size)
    {
      if (m_capacity == 1u) {
        std::uint8_t expected = kSingleReady;
        if (!m_singleState.compare_exchange_strong(
              expected, kSingleBusy, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return false;
        }
        SlotHeader *slot = slotAt(0);
        const std::size_t copySize = std::min<std::size_t>(slot->length, size);
        if (message != nullptr && copySize > 0) { std::memcpy(message, payloadOf(slot), copySize); }
        m_singleState.store(kSingleEmpty, std::memory_order_release);
        return true;
      }

      std::uint64_t position = m_tail.load(std::memory_order_relaxed);
      for (;;) {
        SlotHeader *slot = slotAt(position);
        const std::uint64_t sequence = slot->sequence.load(std::memory_order_acquire);
        const std::int64_t difference = static_cast<std::int64_t>(sequence - (position + 1u));
        if (difference == 0) {
          std::uint64_t expected = position;
          if (m_tail.compare_exchange_weak(expected, position + 1u, std::memory_order_relaxed)) {
            const std::size_t copySize = std::min<std::size_t>(slot->length, size);
            if (message != nullptr && copySize > 0) { std::memcpy(message, payloadOf(slot), copySize); }
            slot->sequence.store(position + m_capacity, std::memory_order_release);
            return true;
          }
          position = expected;
        } else if (difference < 0) {
          return false;// empty
        } else {
          position = m_tail.load(std::memory_order_relaxed);
        }
      }
    }

    [[nodiscard]] bool popDiscard()
    {
      std::byte discard[1]{};
      return pop(discard, sizeof(discard));
    }

    [[nodiscard]] bool empty() const
    {
      if (m_capacity == 1u) { return m_singleState.load(std::memory_order_acquire) != kSingleReady; }
      return m_tail.load(std::memory_order_acquire) == m_head.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const
    {
      if (m_capacity == 1u) { return m_singleState.load(std::memory_order_acquire) == kSingleReady ? 1u : 0u; }
      const std::uint64_t head = m_head.load(std::memory_order_acquire);
      const std::uint64_t tail = m_tail.load(std::memory_order_acquire);
      const std::uint64_t used = head >= tail ? head - tail : 0u;
      return used >= m_capacity ? m_capacity : static_cast<std::size_t>(used);
    }

    [[nodiscard]] bool full() const { return size() >= m_capacity; }

    [[nodiscard]] std::size_t capacity() const { return m_capacity; }

    [[nodiscard]] std::size_t slotSize() const { return m_slotSize; }

    [[nodiscard]] static bool isLockFree() { return std::atomic<std::uint64_t>::is_always_lock_free; }

  private:
    static constexpr std::uint8_t kSingleEmpty = 0;
    static constexpr std::uint8_t kSingleReady = 1;
    static constexpr std::uint8_t kSingleBusy = 2;

    std::atomic<std::uint64_t> m_head{ 0 };
    // Padding keeps m_head and m_tail on separate cache lines to avoid false
    // sharing between producer and consumer. Clang's -Wunused-private-field
    // would flag the arrays, so the intent is documented here and the
    // attributes keep the layout while silencing the warning.
    [[maybe_unused]] std::byte m_headPadding[kCacheLine - sizeof(std::uint64_t)]{};
    std::atomic<std::uint64_t> m_tail{ 0 };
    [[maybe_unused]] std::byte m_tailPadding[kCacheLine - sizeof(std::uint64_t)]{};
    std::atomic<std::uint8_t> m_singleState{ kSingleEmpty };
    std::size_t m_capacity = 0;
    std::size_t m_slotSize = 0;
    std::size_t m_stride = 0;
    ShmSegmentAccessor m_segment;
    std::byte *m_allocation = nullptr;
    std::intptr_t m_bufferOffset = 0;

    /** Base of the aligned slot array in this process' mapping. Derived from
     *  `this` by pointer arithmetic on std::byte (element size 1), so it needs
     *  neither an integer-to-pointer cast nor a byte count scaled by a wider
     *  element type. */
    [[nodiscard]] std::byte *buffer() const
    {
      return reinterpret_cast<std::byte *>(const_cast<ShmRing *>(this)) + m_bufferOffset;
    }

    [[nodiscard]] SlotHeader *slotAt(std::uint64_t position) const
    {
      return reinterpret_cast<SlotHeader *>(buffer() + (position % m_capacity) * m_stride);
    }

    /** Payload of a slot lives directly behind its header; byte arithmetic
     *  keeps the offset unambiguous. */
    [[nodiscard]] static std::byte *payloadOf(SlotHeader *slot)
    {
      return reinterpret_cast<std::byte *>(slot) + kSlotHeaderSize;
    }
  };

  /** State shared by every core of one process that opens the same queue. */
  struct QueueState
  {
    ShmRing *ring = nullptr;
    void *ringAllocation = nullptr;
    ShmSegmentAccessor accessor;
    std::string name;
    int32_t capacity = 0;
    std::size_t slotSize = 0;
    std::mutex waitMutex;
    std::condition_variable waitCondition;

    explicit QueueState(ShmSegmentAccessor segment) : accessor(segment) {}

    ~QueueState()
    {
      if (ring != nullptr) {
        ring->~ShmRing();
        ring = nullptr;
      }
      if (ringAllocation != nullptr) {
        accessor.deallocate(ringAllocation);
        ringAllocation = nullptr;
      }
    }

    void notifyWaiters() { waitCondition.notify_all(); }

    void waitForProgress(const std::chrono::steady_clock::time_point &deadline, bool bounded)
    {
      std::unique_lock<std::mutex> lock(waitMutex);
      if (!bounded) {
        waitCondition.wait_for(lock, kWaitSlice);
        return;
      }
      const auto now = std::chrono::steady_clock::now();
      if (now >= deadline) { return; }
      const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
      waitCondition.wait_for(lock, std::min(remaining, kWaitSlice));
    }
  };

  std::mutex g_registryMutex;
  std::map<std::string, std::weak_ptr<QueueState>> g_registry;

  [[nodiscard]] std::shared_ptr<QueueState>
    createState(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
  {
    auto state = std::make_shared<QueueState>(segment);
    state->name = name;
    state->capacity = msgCount;
    state->slotSize = msgSize;

    const std::size_t allocationSize = sizeof(ShmRing) + kCacheLine - 1u;
    void *raw = segment.allocate(allocationSize);
    if (raw == nullptr) { throw std::bad_alloc(); }
    state->ringAllocation = raw;

    void *alignedRaw = raw;
    std::size_t alignmentSpace = allocationSize;
    if (std::align(alignof(ShmRing), sizeof(ShmRing), alignedRaw, alignmentSpace) == nullptr) {
      throw std::bad_alloc();
    }

    try {
      state->ring = new (alignedRaw) ShmRing(static_cast<std::size_t>(msgCount), msgSize, segment);
    } catch (...) {
      state->ring = nullptr;
      throw;
    }
    return state;
  }

  [[nodiscard]] std::shared_ptr<QueueState>
    openOrCreateState(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
  {
    if (msgCount <= 0) { throw std::invalid_argument("MessageQueueShmCore message count must be positive"); }
    if (msgSize == 0) { throw std::invalid_argument("MessageQueueShmCore message size must be positive"); }

    std::lock_guard<std::mutex> lock(g_registryMutex);
    const auto found = g_registry.find(name);
    if (found != g_registry.end()) {
      if (auto existing = found->second.lock()) {
        if (existing->capacity != msgCount || existing->slotSize != msgSize) {
          throw std::invalid_argument("MessageQueueShmCore already exists with a different layout: " + name);
        }
        return existing;
      }
      g_registry.erase(found);
    }

    auto state = createState(name, msgCount, msgSize, segment);
    g_registry.emplace(name, state);
    return state;
  }

  [[nodiscard]] std::shared_ptr<QueueState> findStateByName(const std::string &name)
  {
    std::lock_guard<std::mutex> lock(g_registryMutex);
    const auto found = g_registry.find(name);
    if (found == g_registry.end()) { return nullptr; }
    return found->second.lock();
  }

  [[nodiscard]] bool pushBlocking(const std::shared_ptr<QueueState> &state, const void *message, std::size_t size)
  {
    for (;;) {
      if (state->ring->push(message, size)) {
        state->notifyWaiters();
        return true;
      }
      state->waitForProgress({}, false);
    }
  }

  [[nodiscard]] bool pushWithTimeout(const std::shared_ptr<QueueState> &state,
    const void *message,
    std::size_t size,
    const std::chrono::milliseconds &timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
      if (state->ring->push(message, size)) {
        state->notifyWaiters();
        return true;
      }
      if (std::chrono::steady_clock::now() >= deadline) { return false; }
      state->waitForProgress(deadline, true);
    }
  }

  [[nodiscard]] bool popBlocking(const std::shared_ptr<QueueState> &state, void *message, std::size_t size)
  {
    for (;;) {
      if (state->ring->pop(message, size)) {
        state->notifyWaiters();
        return true;
      }
      state->waitForProgress({}, false);
    }
  }

  [[nodiscard]] bool popWithTimeout(const std::shared_ptr<QueueState> &state,
    void *message,
    std::size_t size,
    const std::chrono::milliseconds &timeout)
  {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
      if (state->ring->pop(message, size)) {
        state->notifyWaiters();
        return true;
      }
      if (std::chrono::steady_clock::now() >= deadline) { return false; }
      state->waitForProgress(deadline, true);
    }
  }

  void validateSize(const QueueState &state, std::size_t size)
  {
    if (size > state.slotSize) { throw std::length_error("message exceeds MessageQueueShmCore slot size"); }
  }

}// namespace mqshm_detail

using mqshm_detail::findStateByName;
using mqshm_detail::g_registry;
using mqshm_detail::g_registryMutex;
using mqshm_detail::openOrCreateState;
using mqshm_detail::popBlocking;
using mqshm_detail::popWithTimeout;
using mqshm_detail::pushBlocking;
using mqshm_detail::pushWithTimeout;
using mqshm_detail::QueueState;
using mqshm_detail::ShmRing;
using mqshm_detail::validateSize;

struct MessageQueueShmCore::Impl
{
  std::shared_ptr<QueueState> state;
  MessageQueueOverflowPolicy policy = MessageQueueOverflowPolicy::BlockWithTimeout;
  std::chrono::milliseconds timeout{ 100 };
  std::string deadLetterQueueName;
  /** Kept open so the forwarded messages stay visible/receivable after this call. */
  std::shared_ptr<QueueState> deadLetterState;

  Impl(const std::string &name, int32_t msgCount, std::size_t msgSize, ShmSegmentAccessor segment)
    : state(openOrCreateState(name, msgCount, msgSize, segment))
  {}

  [[nodiscard]] bool forwardToDeadLetter(const void *message, std::size_t size)
  {
    if (deadLetterQueueName.empty() || deadLetterQueueName == state->name) { return false; }
    try {
      if (deadLetterState == nullptr || deadLetterState->name != deadLetterQueueName) {
        deadLetterState = openOrCreateState(deadLetterQueueName, state->capacity, state->slotSize, state->accessor);
      }
      if (deadLetterState->ring->push(message, size)) {
        deadLetterState->notifyWaiters();
        return true;
      }
      return false;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] bool applyOverflowPolicy(const void *message, std::size_t size)
  {
    switch (policy) {
    case MessageQueueOverflowPolicy::DropOldest: {
      while (state->ring->popDiscard()) {
        if (state->ring->push(message, size)) {
          state->notifyWaiters();
          return true;
        }
      }
      return false;
    }
    case MessageQueueOverflowPolicy::BlockWithTimeout:
      // Bounded wait configured via setOverflowTimeout(); if the queue stays
      // full for the whole window the message is dropped (returns false).
      return pushWithTimeout(state, message, size, timeout);
    case MessageQueueOverflowPolicy::DeadLetter:
      return forwardToDeadLetter(message, size);
    case MessageQueueOverflowPolicy::Fail:
    case MessageQueueOverflowPolicy::DropNewest:
    default:
      return false;
    }
  }
};

MessageQueueShmCore::MessageQueueShmCore(const std::string &name,
  int32_t msgCount,
  std::size_t msgSize,
  ShmSegmentAccessor segment)
  : m_impl(std::make_unique<Impl>(name, msgCount, msgSize, segment))
{}

MessageQueueShmCore::~MessageQueueShmCore() = default;

void MessageQueueShmCore::removeOf(const std::string &messageQueueName)
{
  std::lock_guard<std::mutex> lock(g_registryMutex);
  g_registry.erase(messageQueueName);
}

int32_t MessageQueueShmCore::numberMessagesOfIfExistsOf(const std::string &messageQueueName)
{
  const auto state = findStateByName(messageQueueName);
  if (state == nullptr) { return 0; }
  return static_cast<int32_t>(state->ring->size());
}

int32_t MessageQueueShmCore::numberMessagesOf() const { return static_cast<int32_t>(m_impl->state->ring->size()); }

const std::string &MessageQueueShmCore::getName() const { return m_impl->state->name; }

int32_t MessageQueueShmCore::getMsgCount() const { return m_impl->state->capacity; }

std::size_t MessageQueueShmCore::getMsgSize() const { return m_impl->state->slotSize; }

bool MessageQueueShmCore::isLockFree() { return ShmRing::isLockFree(); }

void MessageQueueShmCore::sendOf(const void *message, std::size_t size)
{
  validateSize(*m_impl->state, size);
  static_cast<void>(pushBlocking(m_impl->state, message, size));
}

bool MessageQueueShmCore::trySendOf(const void *message, std::size_t size)
{
  validateSize(*m_impl->state, size);
  if (m_impl->state->ring->push(message, size)) {
    m_impl->state->notifyWaiters();
    return true;
  }
  return m_impl->applyOverflowPolicy(message, size);
}

bool MessageQueueShmCore::sendOfWithTimeout(const void *message,
  std::size_t size,
  const std::chrono::milliseconds &timeout)
{
  validateSize(*m_impl->state, size);
  if (pushWithTimeout(m_impl->state, message, size, timeout)) { return true; }
  return m_impl->applyOverflowPolicy(message, size);
}

void MessageQueueShmCore::receiveOf(void *message, std::size_t size)
{
  static_cast<void>(popBlocking(m_impl->state, message, size));
}

bool MessageQueueShmCore::tryReceiveOf(void *message, std::size_t size)
{
  if (m_impl->state->ring->pop(message, size)) {
    m_impl->state->notifyWaiters();
    return true;
  }
  return false;
}

bool MessageQueueShmCore::receiveOfWithTimeout(void *message,
  std::size_t size,
  const std::chrono::milliseconds &timeout)
{
  return popWithTimeout(m_impl->state, message, size, timeout);
}

void MessageQueueShmCore::setOverflowPolicy(MessageQueueOverflowPolicy policy) { m_impl->policy = policy; }

void MessageQueueShmCore::setOverflowTimeout(const std::chrono::milliseconds &timeout) { m_impl->timeout = timeout; }

void MessageQueueShmCore::setDeadLetterQueueName(const std::string &name) { m_impl->deadLetterQueueName = name; }

}// namespace lintel
