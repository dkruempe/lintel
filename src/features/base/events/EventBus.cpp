#include "base_library/features/base/events/EventBus.h"

#include <chrono>
#include <cstring>
#include <new>
#include <stdexcept>
#include <thread>

#include <unistd.h>

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "EventBus requires lock-free 64 bit atomics");
static_assert(std::atomic<std::size_t>::is_always_lock_free,
              "EventBus requires lock-free size_t atomics");

EventBus::EventBus(const std::string &name, const EventBusConfig &config,
                   ShmSegmentManager *segmentManager)
    : m_busCapacity(config.busCapacity),
      m_subscriberCapacity(config.subscriberCapacity),
      m_maxTopics(config.maxTopics),
      m_maxSubscribers(config.maxSubscribers),
      m_bus(config.busCapacity, segmentManager) {
    if (!config.isValid()) {
        throw std::invalid_argument("invalid event bus configuration");
    }
    copyInto(m_name, name);
    std::size_t built = 0;
    try {
        for (; built < EventBusLimits::MAX_SUBSCRIBERS; ++built) {
            new (&m_subscriberQueues[built])
                    ShmRing(config.subscriberCapacity, segmentManager);
        }
    } catch (...) {
        for (std::size_t i = 0; i < built; ++i) {
            reinterpret_cast<ShmRing *>(&m_subscriberQueues[i])->~ShmRing();
        }
        throw;
    }
}

EventBus::~EventBus() {
    for (std::size_t i = 0; i < EventBusLimits::MAX_SUBSCRIBERS; ++i) {
        reinterpret_cast<ShmRing *>(&m_subscriberQueues[i])->~ShmRing();
    }
}

const char *EventBus::getName() const {
    return m_name;
}

EventBusConfig EventBus::getConfig() const {
    EventBusConfig config;
    config.busCapacity = m_busCapacity;
    config.subscriberCapacity = m_subscriberCapacity;
    config.maxTopics = m_maxTopics;
    config.maxSubscribers = m_maxSubscribers;
    return config;
}

bool EventBus::isLockFree() const {
    return ShmRing::isLockFree();
}

int EventBus::findTopicIndex(const char *topic) const {
    const std::size_t count = m_topicCount.load(std::memory_order_relaxed);
    const std::size_t limit = count < EventBusLimits::MAX_TOPICS
                                      ? count
                                      : EventBusLimits::MAX_TOPICS;
    for (std::size_t i = 0; i < limit; ++i) {
        if (nameEquals(m_topics[i].name, topic)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int EventBus::findSubscriberIndex(const char *subscriber) const {
    const std::size_t count = m_subscriberCount.load(std::memory_order_relaxed);
    const std::size_t limit = count < EventBusLimits::MAX_SUBSCRIBERS
                                      ? count
                                      : EventBusLimits::MAX_SUBSCRIBERS;
    for (std::size_t i = 0; i < limit; ++i) {
        if (nameEquals(m_subscribers[i].name, subscriber)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

EventBus::ShmRing &EventBus::subscriberQueueOf(std::size_t index) {
    return *reinterpret_cast<ShmRing *>(&m_subscriberQueues[index]);
}

const EventBus::ShmRing &EventBus::subscriberQueueOf(std::size_t index) const {
    return *reinterpret_cast<const ShmRing *>(&m_subscriberQueues[index]);
}

void EventBus::copyInto(char (&destination)[Event::NAME_SIZE],
                        const std::string &value) {
    const std::size_t length = value.size() >= Event::NAME_SIZE
                                       ? Event::NAME_SIZE - 1
                                       : value.size();
    std::memcpy(destination, value.data(), length);
    destination[length] = '\0';
}

bool EventBus::nameEquals(const char *lhs, const char *rhs) {
    return std::strncmp(lhs, rhs, Event::NAME_SIZE) == 0;
}

EventBus::ShmRing::ShmRing(std::size_t capacity,
                           ShmSegmentManager *segmentManager)
    : m_capacity(capacity),
      m_segmentManager(segmentManager) {
    if (capacity == 0) {
        throw std::invalid_argument("event bus queue capacity must be positive");
    }
    if (segmentManager == nullptr) {
        throw std::invalid_argument("event bus segment manager must not be null");
    }
    const std::size_t bufferSize = capacity * sizeof(Slot);
    void *raw = segmentManager->allocate(bufferSize + CACHE_LINE - 1u);
    if (raw == nullptr) {
        throw std::bad_alloc();
    }
    m_allocation = raw;
    void *aligned = reinterpret_cast<void *>(
            (reinterpret_cast<std::uintptr_t>(raw) + CACHE_LINE - 1u) &
            ~(CACHE_LINE - 1u));
    m_bufferOffset = static_cast<char *>(aligned) -
                     reinterpret_cast<char *>(this);
    Slot *slots = buffer();
    for (std::size_t i = 0; i < capacity; ++i) {
        new (&slots[i]) Slot();
        slots[i].sequence.store(static_cast<std::uint64_t>(i),
                                std::memory_order_relaxed);
    }
}

EventBus::ShmRing::~ShmRing() {
    if (m_allocation != nullptr && m_segmentManager != nullptr) {
        m_segmentManager->deallocate(m_allocation);
        m_allocation = nullptr;
    }
}

EventBus::ShmRing::Slot *EventBus::ShmRing::buffer() {
    return reinterpret_cast<Slot *>(
            reinterpret_cast<std::uintptr_t>(this) +
            static_cast<std::uintptr_t>(m_bufferOffset));
}

const EventBus::ShmRing::Slot *EventBus::ShmRing::buffer() const {
    return reinterpret_cast<const Slot *>(
            reinterpret_cast<std::uintptr_t>(this) +
            static_cast<std::uintptr_t>(m_bufferOffset));
}

bool EventBus::ShmRing::push(const Event &event) {
    Slot *slots = buffer();
    std::uint64_t pos = m_head.load(std::memory_order_relaxed);
    for (;;) {
        Slot &slot = slots[pos % m_capacity];
        const std::uint64_t seq = slot.sequence.load(std::memory_order_acquire);
        const std::int64_t dif = static_cast<std::int64_t>(seq - pos);
        if (dif == 0) {
            std::uint64_t expected = pos;
            if (m_head.compare_exchange_weak(expected, pos + 1u,
                                             std::memory_order_relaxed)) {
                slot.value = event;
                slot.sequence.store(pos + 1u, std::memory_order_release);
                return true;
            }
            pos = expected;
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_head.load(std::memory_order_relaxed);
        }
    }
}

bool EventBus::ShmRing::pop(Event &out) {
    Slot *slots = buffer();
    std::uint64_t pos = m_tail.load(std::memory_order_relaxed);
    for (;;) {
        Slot &slot = slots[pos % m_capacity];
        const std::uint64_t seq = slot.sequence.load(std::memory_order_acquire);
        const std::int64_t dif = static_cast<std::int64_t>(seq - (pos + 1u));
        if (dif == 0) {
            std::uint64_t expected = pos;
            if (m_tail.compare_exchange_weak(expected, pos + 1u,
                                             std::memory_order_relaxed)) {
                out = slot.value;
                slot.sequence.store(pos + m_capacity, std::memory_order_release);
                return true;
            }
            pos = expected;
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_tail.load(std::memory_order_relaxed);
        }
    }
}

bool EventBus::ShmRing::empty() const {
    return m_tail.load(std::memory_order_acquire) ==
           m_head.load(std::memory_order_acquire);
}

bool EventBus::ShmRing::isLockFree() {
    return std::atomic<std::uint64_t>::is_always_lock_free;
}

bool EventBus::registerTopic(const std::string &topic) {
    if (topic.empty()) {
        return false;
    }
    m_registryLock.acquire();
    if (findTopicIndex(topic.c_str()) >= 0) {
        m_registryLock.release();
        return true;
    }
    const std::size_t count = m_topicCount.load(std::memory_order_relaxed);
    if (count >= m_maxTopics) {
        m_registryLock.release();
        return false;
    }
    std::size_t slot = EventBusLimits::MAX_TOPICS;
    for (std::size_t i = 0; i < EventBusLimits::MAX_TOPICS; ++i) {
        if (m_topics[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }
    if (slot == EventBusLimits::MAX_TOPICS) {
        m_registryLock.release();
        return false;
    }
    copyInto(m_topics[slot].name, topic);
    m_topics[slot].subscriberCount.store(0u, std::memory_order_relaxed);
    m_topicCount.store(count + 1u, std::memory_order_relaxed);
    m_registryLock.release();
    return true;
}

bool EventBus::isTopicRegistered(const std::string &topic) const {
    m_registryLock.acquire();
    const bool found = findTopicIndex(topic.c_str()) >= 0;
    m_registryLock.release();
    return found;
}

std::size_t EventBus::topicCount() const {
    m_registryLock.acquire();
    const std::size_t count = m_topicCount.load(std::memory_order_relaxed);
    m_registryLock.release();
    return count;
}

bool EventBus::subscribe(const std::string &topic,
                         const std::string &subscriber) {
    if (topic.empty() || subscriber.empty()) {
        return false;
    }
    m_registryLock.acquire();
    const int topicIndex = findTopicIndex(topic.c_str());
    if (topicIndex < 0) {
        m_registryLock.release();
        return false;
    }
    const int existingSubscriber = findSubscriberIndex(subscriber.c_str());
    std::size_t subscriberSlot = 0;
    if (existingSubscriber >= 0) {
        subscriberSlot = static_cast<std::size_t>(existingSubscriber);
        if (!m_subscribers[subscriberSlot].active.load(
                    std::memory_order_relaxed)) {
            m_subscribers[subscriberSlot].active.store(
                    true, std::memory_order_relaxed);
        }
    } else {
        const std::size_t count = m_subscriberCount.load(std::memory_order_relaxed);
        if (count >= m_maxSubscribers) {
            m_registryLock.release();
            return false;
        }
        std::size_t slot = EventBusLimits::MAX_SUBSCRIBERS;
        for (std::size_t i = 0; i < EventBusLimits::MAX_SUBSCRIBERS; ++i) {
            if (m_subscribers[i].name[0] == '\0') {
                slot = i;
                break;
            }
        }
        if (slot == EventBusLimits::MAX_SUBSCRIBERS) {
            m_registryLock.release();
            return false;
        }
        copyInto(m_subscribers[slot].name, subscriber);
        m_subscribers[slot].active.store(true, std::memory_order_relaxed);
        m_subscriberCount.store(count + 1u, std::memory_order_relaxed);
        subscriberSlot = slot;
    }
    TopicEntry &topicEntry = m_topics[static_cast<std::size_t>(topicIndex)];
    const std::size_t subscriberCount =
            topicEntry.subscriberCount.load(std::memory_order_relaxed);
    for (std::size_t k = 0; k < subscriberCount; ++k) {
        if (topicEntry.subscribers[k] == subscriberSlot) {
            m_registryLock.release();
            return true;
        }
    }
    if (subscriberCount >= m_maxSubscribers) {
        m_registryLock.release();
        return false;
    }
    topicEntry.subscribers[subscriberCount] = subscriberSlot;
    topicEntry.subscriberCount.store(subscriberCount + 1u,
                                     std::memory_order_release);
    m_subscribers[subscriberSlot].topicRefCount.fetch_add(
            1u, std::memory_order_relaxed);
    m_registryLock.release();
    return true;
}

bool EventBus::unsubscribe(const std::string &topic,
                           const std::string &subscriber) {
    m_registryLock.acquire();
    const int topicIndex = findTopicIndex(topic.c_str());
    if (topicIndex < 0) {
        m_registryLock.release();
        return false;
    }
    const int subscriberIndex = findSubscriberIndex(subscriber.c_str());
    if (subscriberIndex < 0) {
        m_registryLock.release();
        return false;
    }
    TopicEntry &topicEntry = m_topics[static_cast<std::size_t>(topicIndex)];
    std::size_t subscriberCount =
            topicEntry.subscriberCount.load(std::memory_order_relaxed);
    const std::size_t target = static_cast<std::size_t>(subscriberIndex);
    bool removed = false;
    for (std::size_t k = 0; k < subscriberCount; ++k) {
        if (topicEntry.subscribers[k] == target) {
            for (std::size_t j = k; j + 1u < subscriberCount; ++j) {
                topicEntry.subscribers[j] = topicEntry.subscribers[j + 1u];
            }
            --subscriberCount;
            topicEntry.subscriberCount.store(subscriberCount,
                                             std::memory_order_release);
            removed = true;
            break;
        }
    }
    if (removed) {
        SubscriberEntry &entry = m_subscribers[static_cast<std::size_t>(subscriberIndex)];
        const std::size_t remaining =
                entry.topicRefCount.fetch_sub(1u, std::memory_order_relaxed) - 1u;
        if (remaining == 0) {
            entry.active.store(false, std::memory_order_relaxed);
        }
    }
    m_registryLock.release();
    return removed;
}

bool EventBus::isSubscribed(const std::string &topic,
                            const std::string &subscriber) const {
    m_registryLock.acquire();
    bool subscribed = false;
    const int topicIndex = findTopicIndex(topic.c_str());
    if (topicIndex >= 0) {
        const int subscriberIndex = findSubscriberIndex(subscriber.c_str());
        if (subscriberIndex >= 0) {
            const TopicEntry &topicEntry =
                    m_topics[static_cast<std::size_t>(topicIndex)];
            const std::size_t subscriberCount =
                    topicEntry.subscriberCount.load(std::memory_order_acquire);
            for (std::size_t k = 0; k < subscriberCount; ++k) {
                if (topicEntry.subscribers[k] ==
                    static_cast<std::size_t>(subscriberIndex)) {
                    subscribed = true;
                    break;
                }
            }
        }
    }
    m_registryLock.release();
    return subscribed;
}

std::size_t EventBus::numberOfSubscribersOf(const std::string &topic) const {
    m_registryLock.acquire();
    std::size_t count = 0;
    const int topicIndex = findTopicIndex(topic.c_str());
    if (topicIndex >= 0) {
        count = m_topics[static_cast<std::size_t>(topicIndex)]
                        .subscriberCount.load(std::memory_order_acquire);
    }
    m_registryLock.release();
    return count;
}

std::size_t EventBus::subscriberCount() const {
    m_registryLock.acquire();
    const std::size_t count = m_subscriberCount.load(std::memory_order_relaxed);
    m_registryLock.release();
    return count;
}

bool EventBus::publish(const Event &event) {
    Event copy = event;
    copy.setSequence(m_sequence.fetch_add(1, std::memory_order_relaxed) + 1);
    if (m_bus.push(copy)) {
        return true;
    }
    m_droppedEvents.fetch_add(1u, std::memory_order_relaxed);
    return false;
}

bool EventBus::publishToSubscribers(const Event &event) {
    Event copy = event;
    copy.setSequence(m_sequence.fetch_add(1, std::memory_order_relaxed) + 1);
    m_registryLock.acquire();
    const int topicIndex = findTopicIndex(copy.topic());
    if (topicIndex < 0) {
        m_registryLock.release();
        return false;
    }
    TopicEntry &topicEntry = m_topics[static_cast<std::size_t>(topicIndex)];
    const std::size_t subscriberCount =
            topicEntry.subscriberCount.load(std::memory_order_acquire);
    std::size_t indices[EventBusLimits::MAX_SUBSCRIBERS]{};
    for (std::size_t k = 0; k < subscriberCount; ++k) {
        indices[k] = topicEntry.subscribers[k];
    }
    m_registryLock.release();
    bool delivered = false;
    for (std::size_t k = 0; k < subscriberCount; ++k) {
        if (subscriberQueueOf(indices[k]).push(copy)) {
            delivered = true;
        } else {
            m_droppedEvents.fetch_add(1u, std::memory_order_relaxed);
        }
    }
    return delivered;
}

std::optional<Event> EventBus::receive() {
    Event out{};
    if (m_bus.pop(out)) {
        return out;
    }
    return std::nullopt;
}

std::optional<Event> EventBus::receiveOf(const std::string &subscriber) {
    m_registryLock.acquire();
    const int subscriberIndex = findSubscriberIndex(subscriber.c_str());
    m_registryLock.release();
    if (subscriberIndex < 0) {
        return std::nullopt;
    }
    Event out{};
    if (subscriberQueueOf(static_cast<std::size_t>(subscriberIndex)).pop(out)) {
        return out;
    }
    return std::nullopt;
}

bool EventBus::isEmpty() const {
    return m_bus.empty();
}

bool EventBus::isEmptyOf(const std::string &subscriber) const {
    m_registryLock.acquire();
    const int subscriberIndex = findSubscriberIndex(subscriber.c_str());
    m_registryLock.release();
    if (subscriberIndex < 0) {
        return true;
    }
    return subscriberQueueOf(static_cast<std::size_t>(subscriberIndex)).empty();
}

std::int64_t EventBus::sequenceOf() const {
    return m_sequence.load(std::memory_order_relaxed);
}

std::uint64_t EventBus::droppedEventsOf() const {
    return m_droppedEvents.load(std::memory_order_relaxed);
}

void EventBus::reset() {
    m_registryLock.acquire();
    Event drop{};
    while (m_bus.pop(drop)) {
    }
    for (std::size_t i = 0; i < EventBusLimits::MAX_SUBSCRIBERS; ++i) {
        while (subscriberQueueOf(i).pop(drop)) {
        }
    }
    m_registryLock.release();
}

std::size_t EventBus::requiredSize(const EventBusConfig &config) {
    if (!config.isValid()) {
        return 0;
    }
    static constexpr std::size_t CACHELINE = 64;
    const std::size_t slotSize =
            (sizeof(Event) + sizeof(std::atomic<std::uint64_t>) + CACHELINE - 1u) &
            ~(CACHELINE - 1u);
    const std::size_t busRing = config.busCapacity * slotSize + CACHELINE;
    const std::size_t subscriberRing =
            config.subscriberCapacity * slotSize + CACHELINE;
    const std::size_t total = sizeof(EventBus) + busRing +
                              EventBusLimits::MAX_SUBSCRIBERS * subscriberRing;
    static constexpr std::size_t OVERHEAD = 256 * 1024;
    return total + OVERHEAD;
}

EventBus::RegistryLock::RegistryLock() : m_state(0) {}

std::uint64_t EventBus::RegistryLock::ownerId() {
    return static_cast<std::uint64_t>(::getpid()) + 1u;
}

std::uint64_t EventBus::RegistryLock::nowMs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now);
    return static_cast<std::uint64_t>(ms.count());
}

std::uint64_t EventBus::RegistryLock::encode() {
    return (ownerId() << 32) | (nowMs() & TIMESTAMP_MASK);
}

bool EventBus::RegistryLock::isStale(std::uint64_t state) {
    const std::uint64_t timestamp = state & TIMESTAMP_MASK;
    const std::uint64_t now = nowMs() & TIMESTAMP_MASK;
    const std::uint64_t elapsed = now - timestamp;
    return elapsed > STALE_THRESHOLD_MS;
}

void EventBus::RegistryLock::acquire() const {
    for (;;) {
        std::uint64_t state = m_state.load(std::memory_order_acquire);
        if (state == 0) {
            if (m_state.compare_exchange_weak(state, encode(),
                                              std::memory_order_acquire)) {
                return;
            }
            continue;
        }
        const std::uint64_t owner = state >> 32;
        if (owner != ownerId() && isStale(state)) {
            m_state.compare_exchange_strong(state, 0, std::memory_order_relaxed);
            continue;
        }
        std::this_thread::yield();
    }
}

void EventBus::RegistryLock::release() const {
    m_state.store(0, std::memory_order_release);
}
