#ifndef MESSAGEQUEUEDTOS_H
#define MESSAGEQUEUEDTOS_H
#include <base_library/core/models/JsonSerializable.h>
#include <vector>

#include "MessageQueueDto.h"

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

#endif //MESSAGEQUEUEDTOS_H
