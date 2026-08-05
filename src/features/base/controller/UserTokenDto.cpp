#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/controller/UserTokenDto.h"

#include "base_library/core/services/LoggerService.h"

UserTokenDto::Shapes UserTokenDto::shape{};

UserTokenDto::UserTokenDto(std::string id) : m_id(std::move(id)) {}

void UserTokenDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // ID
    writer->String(shape.ID);
    writer->String(m_id.c_str());
    writer->EndObject();
}

bool UserTokenDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    if (obj.HasMember(shape.ID)) {
        m_id = obj[shape.ID].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", shape.ID);
    }
    return success;
}

const std::string &UserTokenDto::getId() const { return m_id; }
