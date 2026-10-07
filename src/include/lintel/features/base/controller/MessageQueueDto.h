#ifndef LINTEL_MESSAGEQUEUEDTO_H
#define LINTEL_MESSAGEQUEUEDTO_H

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <cstdint>
#include <string>

/** DTO representing a single message queue with its current state */
class MessageQueueDto : public JsonSerializable {
    /** The queue name */
    std::string m_name;
    /** The owning process name */
    std::string m_process;
    /** The maximum number of messages */
    int32_t m_maxMessages{};
    /** The current number of messages */
    int32_t m_messages{};

    /** JSON field name constants */
    static struct Shapes {
        const char *const NAME = "NAME";
        const char *const PROCESS = "PROCESS";
        const char *const MAX_MESSAGES = "MAX_MESSAGES";
        const char *const MESSAGES = "MESSAGES";
    } m_shape;

public:
    /** Default constructor */
    MessageQueueDto() = default;

    /** Construct from a MessageQueueEntry and current message count
     * @param messsageQueueEntry The configuration entry
     * @param messages The current message count */
    MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, int32_t messages);

    /** Get the queue name
     * @return The name */
    [[nodiscard]] const std::string &getName() const;

    /** Get the owning process name
     * @return The process name */
    [[nodiscard]] const std::string &getProcess() const;

    /** Get the maximum message count
     * @return The max messages */
    [[nodiscard]] int32_t getMaxMessages() const;

    /** Get the current message count
     * @return The current messages */
    [[nodiscard]] int32_t getMessages() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif //LINTEL_MESSAGEQUEUEDTO_H
