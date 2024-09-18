#include "base_library/features/base/controller/MessageQueueDtos.h"

MessageQueueDtos::MessageQueueDtos(const std::vector<std::pair<MessageQueueEntry, int32_t> > &messageQueueDtos)
    : m_messageQueueDtos(build(messageQueueDtos)) {
}

std::vector<MessageQueueDto> MessageQueueDtos::build(const std::vector<std::pair<MessageQueueEntry, int32_t> > &pairs) {
    std::vector<MessageQueueDto> messageQueueDtos;
    messageQueueDtos.reserve(pairs.size());
    for (const auto &[entry, messages]: pairs) {
        messageQueueDtos.emplace_back(entry, messages);
    }
    return messageQueueDtos;
}

const std::vector<MessageQueueDto> &MessageQueueDtos::getMessageQueueDtos() const {
    return m_messageQueueDtos;
}

void MessageQueueDtos::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartArray();
    for (const auto &messageQueueDto: m_messageQueueDtos) {
        messageQueueDto.serialize(writer);
    }
    writer->EndArray();
}

void MessageQueueDtos::deserialize(const std::string &json) {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (!doc.IsArray()) {
        return;
    }
    for (const auto &messageQueue: doc.GetArray()) {
        MessageQueueDto messageQueueDto;
        messageQueueDto.deserialize(messageQueue);
        m_messageQueueDtos.push_back(messageQueueDto);
    }
}
