#ifndef MESSAGEQUEUEDTOS_H
#define MESSAGEQUEUEDTOS_H
#include <base_library/core/models/JsonSerializable.h>
#include <vector>

#include "MessageQueueDto.h"

class MessageQueueDtos : public JsonSerializable {
private:
    std::vector<MessageQueueDto> m_messageQueueDtos;

public:
    MessageQueueDtos() = default;

    std::vector<MessageQueueDto> build(const std::vector<std::pair<MessageQueueEntry, int32_t> > &pairs);

    MessageQueueDtos(const std::vector<std::pair<MessageQueueEntry, int32_t> > &messageQueueDtos);

    [[nodiscard]] const std::vector<MessageQueueDto> &getMessageQueueDtos() const;

    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    void deserialize(const std::string &json) override;
};

#endif //MESSAGEQUEUEDTOS_H
