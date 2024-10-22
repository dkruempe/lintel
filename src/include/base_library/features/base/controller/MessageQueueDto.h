#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEDTO_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/configuration/MessageQueueEntry.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <cstdint>
#include <string>

class MessageQueueDto : public JsonSerializable {
    std::string m_name;
    std::string m_process;
    int32_t m_maxMessages{};
    int32_t m_messages{};

    static struct Shapes {
        const std::string NAME = "NAME";
        const std::string PROCESS = "PROCESS";
        const std::string MAX_MESSAGES = "MAX_MESSAGES";
        const std::string MESSAGES = "MESSAGES";
    } m_shape;

public:
    MessageQueueDto() = default;

    MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, int32_t messages);

    [[nodiscard]] const std::string &getName() const;

    [[nodiscard]] const std::string &getProcess() const;

    [[nodiscard]] int32_t getMaxMessages() const;

    [[nodiscard]] int32_t getMessages() const;

    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUEDTO_H
