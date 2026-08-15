#ifndef CPP_BASE_LIBRARY_EVENTBUS_H
#define CPP_BASE_LIBRARY_EVENTBUS_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "base_library/features/base/events/Event.h"

/** Segment manager type of a boost managed mapped file. */
using ShmSegmentManager = boost::interprocess::managed_mapped_file::segment_manager;

/** Compile-time upper bounds for the fixed event bus slots. */
struct EventBusLimits {
    /** Maximum number of topics that can be registered on one bus. */
    static constexpr std::size_t MAX_TOPICS = 16;
    /** Maximum number of subscriber slots available on one bus. */
    static constexpr std::size_t MAX_SUBSCRIBERS = 32;
    /** Maximum queue capacity; boost::lockfree uses a 16 bit index. */
    static constexpr std::size_t MAX_CAPACITY = 65534;
};

/**
 * Runtime configuration of an EventBus. The queue capacities and the maximum
 * topic/subscriber counts are configured when the bus is created; the payload
 * size is fixed at compile time (see Event).
 */
struct EventBusConfig {
    /** Capacity of the point-to-point bus queue. */
    std::size_t busCapacity = 128;
    /** Capacity of every per-subscriber queue. */
    std::size_t subscriberCapacity = 128;
    /** Maximum number of topics that can be registered. */
    std::size_t maxTopics = EventBusLimits::MAX_TOPICS;
    /** Maximum number of subscribers that can be registered. */
    std::size_t maxSubscribers = EventBusLimits::MAX_SUBSCRIBERS;

    /** @return true if all values are within their bounds */
    [[nodiscard]] bool isValid() const {
        return busCapacity > 0 && busCapacity <= EventBusLimits::MAX_CAPACITY &&
               subscriberCapacity > 0 &&
               subscriberCapacity <= EventBusLimits::MAX_CAPACITY &&
               maxTopics > 0 && maxTopics <= EventBusLimits::MAX_TOPICS &&
               maxSubscribers > 0 &&
               maxSubscribers <= EventBusLimits::MAX_SUBSCRIBERS;
    }
};

/**
 * Lock-free, IPC-capable event bus living inside a shared memory segment.
 *
 * The bus offers two delivery modes:
 * - point-to-point: events are pushed into a single multi-producer /
 *   multi-consumer queue and received by calling receive(),
 * - publish/subscribe: events are pushed into the queue of every subscriber
 *   of the event topic (broadcast).
 *
 * All queues are strictly bounded, lock-free MPMC ring buffers with fixed
 * capacity that live inside the shared memory segment. They are IPC-safe:
 * the node buffers are addressed relative to the bus object, so they remain
 * valid regardless of the virtual address a process mapped the segment at.
 * The topic/subscriber registry is guarded by a short spin lock that
 * tolerates crashed processes.
 *
 * The class is stored inside the shared memory segment and therefore must not
 * use virtual functions (vtable pointers would point into the process that
 * constructed the bus). The abstract IEventBus is provided for the local
 * service layer.
 */
class EventBus {
public:

    /**
     * Construct an EventBus. The constructor is only executed when the bus is
     * created for the first time in the segment; later processes find the
     * already constructed object.
     * @param name the name of the bus
     * @param config the runtime configuration
     * @param segmentManager the segment manager to allocate the queues from
     * @throws std::invalid_argument if the configuration is invalid
     */
    EventBus(const std::string &name, const EventBusConfig &config,
             ShmSegmentManager *segmentManager);

    /** Destroys the per-subscriber queues.
     * @note the object normally lives forever inside the segment */
    ~EventBus();

    EventBus(const EventBus &) = delete;
    EventBus &operator=(const EventBus &) = delete;

    /** @return the name of the bus */
    [[nodiscard]] const char *getName() const;

    /** @return the configuration the bus was created with */
    [[nodiscard]] EventBusConfig getConfig() const;

    /** @return true if the underlying queues are lock-free */
    [[nodiscard]] bool isLockFree() const;

    /** Register a topic on the bus (idempotent).
     * @param topic the topic name
     * @return false if the topic table is full */
    bool registerTopic(const std::string &topic);

    /** @param topic the topic name
     * @return true if the topic is registered */
    [[nodiscard]] bool isTopicRegistered(const std::string &topic) const;

    /** @return the number of registered topics */
    [[nodiscard]] std::size_t topicCount() const;

    /** Subscribe a named subscriber to a topic.
     * @param topic the topic name
     * @param subscriber the subscriber name
     * @return false if the topic does not exist or the table is full */
    bool subscribe(const std::string &topic, const std::string &subscriber);

    /** Unsubscribe a named subscriber from a topic.
     * @param topic the topic name
     * @param subscriber the subscriber name
     * @return true if the subscriber was removed */
    bool unsubscribe(const std::string &topic, const std::string &subscriber);

    /** @param topic the topic name
     * @param subscriber the subscriber name
     * @return true if the subscriber is subscribed to the topic */
    [[nodiscard]] bool isSubscribed(const std::string &topic,
                                    const std::string &subscriber) const;

    /** @param topic the topic name
     * @return the number of subscribers of the topic */
    [[nodiscard]] std::size_t numberOfSubscribersOf(
            const std::string &topic) const;

    /** @return the number of reserved subscriber slots */
    [[nodiscard]] std::size_t subscriberCount() const;

    /** Publish an event into the point-to-point bus queue.
     * @param event the event to publish
     * @return false if the bus queue is full */
    bool publish(const Event &event);

    /** Publish an event to every subscriber of the event topic.
     * @param event the event to publish
     * @return false if no subscriber received the event */
    bool publishToSubscribers(const Event &event);

    /** Receive the next event from the point-to-point bus queue.
     * @return the event or std::nullopt if the queue is empty */
    [[nodiscard]] std::optional<Event> receive();

    /** Receive the next event of a subscriber queue.
     * @param subscriber the subscriber name
     * @return the event or std::nullopt if the queue is empty */
    [[nodiscard]] std::optional<Event> receiveOf(
            const std::string &subscriber);

    /** @return true if the point-to-point bus queue is empty */
    [[nodiscard]] bool isEmpty() const;

    /** @param subscriber the subscriber name
     * @return true if the subscriber queue is empty or unknown */
    [[nodiscard]] bool isEmptyOf(const std::string &subscriber) const;

    /** @return the last assigned sequence number */
    [[nodiscard]] std::int64_t sequenceOf() const;

    /** @return the number of events that were dropped due to full queues */
    [[nodiscard]] std::uint64_t droppedEventsOf() const;

    /** Drop all queued events on the bus and every subscriber queue. */
    void reset();

    /** Compute the shared memory size needed for a given configuration.
     * @param config the runtime configuration
     * @return the required segment size in bytes, 0 if the config is invalid */
    static std::size_t requiredSize(const EventBusConfig &config);

private:
    /**
     * Bounded, lock-free, multi-producer/multi-consumer ring buffer.
     *
     * The queue is a Dmitry Vyukov style MPMC queue based on sequence numbers.
     * The mutable state (head/tail positions and the per-slot sequences) lives
     * inside the EventBus object, which is stored in the shared memory
     * segment. The node buffer is allocated from the segment during
     * construction and is addressed through a byte offset relative to this
     * object; the relative layout is identical in every mapping, so the queue
     * works from every process regardless of the virtual address it mapped
     * the segment at.
     */
    class ShmRing {
    public:
        /** Cache line size used to separate hot atomics and align slots. */
        static constexpr std::size_t CACHE_LINE = 64;

        /** A single ring slot; padded to one cache line. */
        struct alignas(CACHE_LINE) Slot {
            std::atomic<std::uint64_t> sequence{0};
            Event value;
        };

        /** Construct an uninitialized ring; use init() before pushing. */
        ShmRing() = default;

        /** Construct a ring with a fixed capacity.
         * @param capacity the maximum number of queued events
         * @param segmentManager the segment the node buffer is allocated from
         * @throws std::invalid_argument on invalid arguments */
        ShmRing(std::size_t capacity, ShmSegmentManager *segmentManager);

        /** Deallocates the node buffer. */
        ~ShmRing();

        ShmRing(const ShmRing &) = delete;
        ShmRing &operator=(const ShmRing &) = delete;

        /** Push an event; fails when the ring is full.
         * @param event the event to queue
         * @return true if the event was queued */
        bool push(const Event &event);

        /** Pop the oldest event; fails when the ring is empty.
         * @param out receives the event
         * @return true if an event was dequeued */
        bool pop(Event &out);

        /** Best-effort emptiness check; an in-flight push may be invisible.
         * @return true if the ring is (likely) empty */
        [[nodiscard]] bool empty() const;

        /** @return true if the queue operations are lock-free */
        [[nodiscard]] static bool isLockFree();

    private:
        std::atomic<std::uint64_t> m_head{0};
        std::byte m_headPadding[CACHE_LINE - sizeof(std::uint64_t)]{};
        std::atomic<std::uint64_t> m_tail{0};
        std::byte m_tailPadding[CACHE_LINE - sizeof(std::uint64_t)]{};
        std::size_t m_capacity = 0;
        ShmSegmentManager *m_segmentManager = nullptr;
        std::byte *m_allocation = nullptr;
        std::int64_t m_bufferOffset = 0;

        [[nodiscard]] Slot *buffer();
        [[nodiscard]] const Slot *buffer() const;
    };

    /** Per-topic registry entry. */
    struct TopicEntry {
        char name[Event::NAME_SIZE]{};
        std::atomic<std::size_t> subscriberCount{0};
        std::size_t subscribers[EventBusLimits::MAX_SUBSCRIBERS]{};
    };

    /** Per-subscriber registry entry. */
    struct SubscriberEntry {
        char name[Event::NAME_SIZE]{};
        std::atomic<bool> active{false};
        std::atomic<std::size_t> topicRefCount{0};
    };

    /**
     * Spin lock guarding the topic/subscriber registry. Records the owning
     * process id and a timestamp so that the lock can be recovered after a
     * process crash.
     */
    class RegistryLock {
    public:
        RegistryLock();

        /** Acquire the lock; steals it if the owner crashed. */
        void acquire() const;

        /** Release the lock. */
        void release() const;

    private:
        static constexpr std::uint64_t STALE_THRESHOLD_MS = 500;
        static constexpr std::uint64_t TIMESTAMP_MASK = 0xFFFFFFFFu;

        mutable std::atomic<std::uint64_t> m_state;

        static std::uint64_t ownerId();
        static std::uint64_t nowMs();
        static bool isStale(std::uint64_t state);
        static std::uint64_t encode();
    };

    char m_name[Event::NAME_SIZE]{};
    std::size_t m_busCapacity = 0;
    std::size_t m_subscriberCapacity = 0;
    std::size_t m_maxTopics = 0;
    std::size_t m_maxSubscribers = 0;
    std::atomic<std::int64_t> m_sequence{0};
    std::atomic<std::uint64_t> m_droppedEvents{0};
    std::atomic<std::size_t> m_topicCount{0};
    std::atomic<std::size_t> m_subscriberCount{0};
    RegistryLock m_registryLock;
    ShmRing m_bus;
    TopicEntry m_topics[EventBusLimits::MAX_TOPICS]{};
    SubscriberEntry m_subscribers[EventBusLimits::MAX_SUBSCRIBERS]{};
    alignas(ShmRing) std::byte
            m_subscriberQueues[EventBusLimits::MAX_SUBSCRIBERS][sizeof(ShmRing)]{};

    [[nodiscard]] int findTopicIndex(const char *topic) const;
    [[nodiscard]] int findSubscriberIndex(const char *subscriber) const;
    [[nodiscard]] ShmRing &subscriberQueueOf(std::size_t index);
    [[nodiscard]] const ShmRing &subscriberQueueOf(std::size_t index) const;

    static void copyInto(char (&destination)[Event::NAME_SIZE],
                         const std::string &value);
    [[nodiscard]] static bool nameEquals(const char *lhs, const char *rhs);
};

/**
 * Abstract interface of an EventBus used by the local service layer
 * (dependency inversion). Implemented by EventBusView, which forwards to the
 * EventBus object stored in shared memory.
 */
class IEventBus {
public:
    virtual ~IEventBus() = default;

    /** @return the name of the bus */
    [[nodiscard]] virtual const char *getName() const = 0;

    /** @return the configuration the bus was created with */
    [[nodiscard]] virtual EventBusConfig getConfig() const = 0;

    /** @return true if the underlying queues are lock-free */
    [[nodiscard]] virtual bool isLockFree() const = 0;

    /** Register a topic on the bus (idempotent).
     * @param topic the topic name
     * @return false if the topic table is full */
    virtual bool registerTopic(const std::string &topic) = 0;

    /** @param topic the topic name
     * @return true if the topic is registered */
    [[nodiscard]] virtual bool isTopicRegistered(
            const std::string &topic) const = 0;

    /** @return the number of registered topics */
    [[nodiscard]] virtual std::size_t topicCount() const = 0;

    /** Subscribe a named subscriber to a topic.
     * @param topic the topic name
     * @param subscriber the subscriber name
     * @return false if the topic does not exist or the table is full */
    virtual bool subscribe(const std::string &topic,
                           const std::string &subscriber) = 0;

    /** Unsubscribe a named subscriber from a topic.
     * @param topic the topic name
     * @param subscriber the subscriber name
     * @return true if the subscriber was removed */
    virtual bool unsubscribe(const std::string &topic,
                             const std::string &subscriber) = 0;

    /** @param topic the topic name
     * @param subscriber the subscriber name
     * @return true if the subscriber is subscribed to the topic */
    [[nodiscard]] virtual bool isSubscribed(
            const std::string &topic,
            const std::string &subscriber) const = 0;

    /** @param topic the topic name
     * @return the number of subscribers of the topic */
    [[nodiscard]] virtual std::size_t numberOfSubscribersOf(
            const std::string &topic) const = 0;

    /** @return the number of reserved subscriber slots */
    [[nodiscard]] virtual std::size_t subscriberCount() const = 0;

    /** Publish an event into the point-to-point bus queue.
     * @param event the event to publish
     * @return false if the bus queue is full */
    virtual bool publish(const Event &event) = 0;

    /** Publish an event to every subscriber of the event topic.
     * @param event the event to publish
     * @return false if no subscriber received the event */
    virtual bool publishToSubscribers(const Event &event) = 0;

    /** Receive the next event from the point-to-point bus queue.
     * @return the event or std::nullopt if the queue is empty */
    [[nodiscard]] virtual std::optional<Event> receive() = 0;

    /** Receive the next event of a subscriber queue.
     * @param subscriber the subscriber name
     * @return the event or std::nullopt if the queue is empty */
    [[nodiscard]] virtual std::optional<Event> receiveOf(
            const std::string &subscriber) = 0;

    /** @return true if the point-to-point bus queue is empty */
    [[nodiscard]] virtual bool isEmpty() const = 0;

    /** @param subscriber the subscriber name
     * @return true if the subscriber queue is empty or unknown */
    [[nodiscard]] virtual bool isEmptyOf(
            const std::string &subscriber) const = 0;

    /** @return the last assigned sequence number */
    [[nodiscard]] virtual std::int64_t sequenceOf() const = 0;

    /** @return the number of events that were dropped due to full queues */
    [[nodiscard]] virtual std::uint64_t droppedEventsOf() const = 0;

    /** Drop all queued events on the bus and every subscriber queue. */
    virtual void reset() = 0;
};

/** Local adapter that exposes a shared memory EventBus through IEventBus. */
class EventBusView : public IEventBus {
public:
    /** Construct a view around a shared memory bus.
     * @param bus the shared memory event bus */
    explicit EventBusView(EventBus &bus) : m_bus(bus) {}

    [[nodiscard]] const char *getName() const override { return m_bus.getName(); }

    [[nodiscard]] EventBusConfig getConfig() const override {
        return m_bus.getConfig();
    }

    [[nodiscard]] bool isLockFree() const override { return m_bus.isLockFree(); }

    bool registerTopic(const std::string &topic) override {
        return m_bus.registerTopic(topic);
    }

    [[nodiscard]] bool isTopicRegistered(
            const std::string &topic) const override {
        return m_bus.isTopicRegistered(topic);
    }

    [[nodiscard]] std::size_t topicCount() const override {
        return m_bus.topicCount();
    }

    bool subscribe(const std::string &topic,
                   const std::string &subscriber) override {
        return m_bus.subscribe(topic, subscriber);
    }

    bool unsubscribe(const std::string &topic,
                     const std::string &subscriber) override {
        return m_bus.unsubscribe(topic, subscriber);
    }

    [[nodiscard]] bool isSubscribed(
            const std::string &topic,
            const std::string &subscriber) const override {
        return m_bus.isSubscribed(topic, subscriber);
    }

    [[nodiscard]] std::size_t numberOfSubscribersOf(
            const std::string &topic) const override {
        return m_bus.numberOfSubscribersOf(topic);
    }

    [[nodiscard]] std::size_t subscriberCount() const override {
        return m_bus.subscriberCount();
    }

    bool publish(const Event &event) override { return m_bus.publish(event); }

    bool publishToSubscribers(const Event &event) override {
        return m_bus.publishToSubscribers(event);
    }

    [[nodiscard]] std::optional<Event> receive() override {
        return m_bus.receive();
    }

    [[nodiscard]] std::optional<Event> receiveOf(
            const std::string &subscriber) override {
        return m_bus.receiveOf(subscriber);
    }

    [[nodiscard]] bool isEmpty() const override { return m_bus.isEmpty(); }

    [[nodiscard]] bool isEmptyOf(
            const std::string &subscriber) const override {
        return m_bus.isEmptyOf(subscriber);
    }

    [[nodiscard]] std::int64_t sequenceOf() const override {
        return m_bus.sequenceOf();
    }

    [[nodiscard]] std::uint64_t droppedEventsOf() const override {
        return m_bus.droppedEventsOf();
    }

    void reset() override { m_bus.reset(); }

private:
    EventBus &m_bus;
};

#endif  // CPP_BASE_LIBRARY_EVENTBUS_H
