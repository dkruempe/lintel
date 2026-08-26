#ifndef MESSAGEQUEUEDTOS_H
#define MESSAGEQUEUEDTOS_H
#include <base_library/core/models/JsonSerializable.h>
#include <optional>
#include <vector>

#include "MessageQueueDto.h"
#include "base_library/features/base/models/Page.h"

/** DTO representing a collection of message queues */
class MessageQueueDtos : public JsonSerializable {
private:
    /** The message queue DTOs */
    std::vector<MessageQueueDto> m_messageQueueDtos;

public:
    /** Default constructor */
    MessageQueueDtos() = default;

    /** Build message queue DTOs from entry/count pairs
     * @param pairs Vector of (MessageQueueEntry, message_count) pairs
     * @return Vector of message queue DTOs */
    static std::vector<MessageQueueDto> build(const std::vector<std::pair<MessageQueueEntry, int32_t> > &pairs);

    /** Construct from entry/count pairs
     * @param messageQueueDtos Vector of (MessageQueueEntry, message_count) pairs */
    MessageQueueDtos(const std::vector<std::pair<MessageQueueEntry, int32_t> > &messageQueueDtos);

    /** Get the message queue DTOs
     * @return Vector of message queue DTOs */
    [[nodiscard]] const std::vector<MessageQueueDto> &getMessageQueueDtos() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;
};

/** DTO representing a single page of keyset-paginated message queues */
class MessageQueuesPageDto : public JsonSerializable {
private:
    /** The message queue DTOs of this page */
    std::vector<MessageQueueDto> m_messageQueueDtos;
    /** True if more pages follow */
    bool m_hasMore = false;
    /** Sort key to pass as 'after' for the next page */
    std::optional<std::string> m_nextAfter;

    /** JSON field name constants */
    static struct Shapes {
        const char *const ITEMS = "items";
        const char *const HAS_MORE = "has_more";
        const char *const NEXT_AFTER = "next_after";
    } shape;

public:
    /** Construct from a paginated result
     * @param page The page of message queue entries */
    explicit MessageQueuesPageDto(const Page<MessageQueueEntry> &page);

    /** Default constructor */
    MessageQueuesPageDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON string
     * @param json The JSON string to parse */
    void deserialize(const std::string &json) override;

    /** Get the message queue DTOs of this page
     * @return Vector of message queue DTOs */
    [[nodiscard]] const std::vector<MessageQueueDto> &getMessageQueueDtos() const;

    /** @return true if more pages follow */
    [[nodiscard]] bool hasMore() const;

    /** @return the continuation key for the next page, if any */
    [[nodiscard]] const std::optional<std::string> &getNextAfter() const;
};

#endif //MESSAGEQUEUEDTOS_H
