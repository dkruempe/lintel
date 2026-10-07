#ifndef LINTEL_EVENT_H
#define LINTEL_EVENT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

/**
 * A fixed-size, trivially copyable event that is exchanged over the EventBus.
 *
 * The event is a POD-like value type: it is safe to store it inside the
 * lock-free shared memory queues and to copy it between processes. The sizes of
 * the topic/type identifiers and of the payload buffer are compile-time
 * constants (matching the "configurable fixed slots" design decision); the
 * queue capacities and topic/subscriber counts are configured at runtime.
 */
class Event {
public:
    /** Maximum length of the topic/subscriber name, including the terminator. */
    static constexpr std::size_t NAME_SIZE = 32;
    /** Maximum payload size in bytes. */
    static constexpr std::size_t CONTENT_SIZE = 256;

    /** Default constructor; zero-initializes the event. */
    Event() = default;

    /**
     * Construct an event with topic and type set.
     * @param topic identifier of the channel this event belongs to
     * @param type  identifier of the event type
     */
    Event(const std::string &topic, const std::string &type) {
        copyInto(m_topic, topic);
        copyInto(m_type, type);
    }

    /** Set the topic (truncated to NAME_SIZE).
     * @param topic the topic name */
    void setTopic(const std::string &topic) { copyInto(m_topic, topic); }

    /** Set the type (truncated to NAME_SIZE).
     * @param type the type name */
    void setType(const std::string &type) { copyInto(m_type, type); }

    /** @return the topic name (NUL-terminated) */
    [[nodiscard]] const char *topic() const { return m_topic; }

    /** @return the type name (NUL-terminated) */
    [[nodiscard]] const char *type() const { return m_type; }

    /** Set the monotonically increasing sequence number.
     * @param sequence the sequence number */
    void setSequence(std::int64_t sequence) { m_sequence = sequence; }

    /** @return the sequence number */
    [[nodiscard]] std::int64_t sequence() const { return m_sequence; }

    /** Set the creation timestamp (Unix epoch seconds).
     * @param timestamp the timestamp */
    void setTimestamp(std::int64_t timestamp) { m_timestamp = timestamp; }

    /** @return the creation timestamp (Unix epoch seconds) */
    [[nodiscard]] std::int64_t timestamp() const { return m_timestamp; }

    /**
     * Copy a payload of given size into the content buffer.
     * @param data pointer to the payload (may be nullptr when size is zero)
     * @param size number of bytes to copy; clamped to CONTENT_SIZE
     */
    void setContent(const std::byte *data, std::size_t size) {
        const std::size_t length = size > CONTENT_SIZE ? CONTENT_SIZE : size;
        if (length > 0 && data != nullptr) {
            std::memcpy(m_content, data, length);
        }
        m_contentLength = length;
    }

    /**
     * Assign an object of type T to the reserved content space.
     * Warning: T must be trivially copyable and must not exceed CONTENT_SIZE.
     * @tparam T type of the object
     * @param content object which gets copied into the content buffer
     */
    template<typename T>
    void assign(const T &content) {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable to be stored in an Event");
        static_assert(sizeof(T) <= CONTENT_SIZE,
                      "content is bigger than reserved size for content");
        std::memcpy(m_content, &content, sizeof(T));
        m_contentLength = sizeof(T);
    }

    /**
     * Convert the content buffer back into an object of type T.
     * @tparam T target type
     * @return the converted object
     */
    template<typename T>
    [[nodiscard]] T as() const {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable to be read from an Event");
        static_assert(sizeof(T) <= CONTENT_SIZE,
                      "content is bigger than reserved size for content");
        T result{};
        std::memcpy(&result, m_content, sizeof(T));
        return result;
    }

    /** @return the number of valid payload bytes */
    [[nodiscard]] std::size_t contentLength() const { return m_contentLength; }

    /** @return pointer to the raw payload bytes */
    [[nodiscard]] const std::byte *content() const {
        return reinterpret_cast<const std::byte *>(m_content);
    }

    /** @return true if the event has no payload */
    [[nodiscard]] bool isContentEmpty() const { return m_contentLength == 0; }

private:
    char m_topic[NAME_SIZE]{};
    char m_type[NAME_SIZE]{};
    char m_content[CONTENT_SIZE]{};
    std::size_t m_contentLength = 0;
    std::int64_t m_sequence = 0;
    std::int64_t m_timestamp = 0;

    static constexpr void copyInto(char (&destination)[NAME_SIZE],
                         std::string_view value) {
        const std::size_t length =
                value.size() >= NAME_SIZE ? NAME_SIZE - 1 : value.size();
        std::memcpy(destination, value.data(), length);
        destination[length] = '\0';
    }
};

static_assert(std::is_trivially_copyable_v<Event>,
              "Event must stay trivially copyable for the lock-free queues");

#endif  // LINTEL_EVENT_H
