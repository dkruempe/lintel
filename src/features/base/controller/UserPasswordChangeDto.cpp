#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/controller/UserPasswordChangeDto.h"

#include <utility>

#include "base_library/core/services/LoggerService.h"

UserPasswordChangeDto::Shapes UserPasswordChangeDto::shape{};

UserPasswordChangeDto::UserPasswordChangeDto(std::string userName,
                                             std::string oldPassword,
                                             std::string newPassword)
        : m_userName(std::move(userName)),
          m_oldPassword(std::move(oldPassword)),
          m_newPassword(std::move(newPassword)) {}

const std::string &UserPasswordChangeDto::getUserName() const {
    return m_userName;
}

const std::string &UserPasswordChangeDto::getOldPassword() const {
    return m_oldPassword;
}

const std::string &UserPasswordChangeDto::getNewPassword() const {
    return m_newPassword;
}

void UserPasswordChangeDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    writer->String(shape.USER_NAME);
    writer->String(m_userName.c_str());
    writer->String(shape.OLD_PASSWORD);
    writer->String(m_oldPassword.c_str());
    writer->String(shape.NEW_PASSWORD);
    writer->String(m_newPassword.c_str());
    writer->EndObject();
}

bool UserPasswordChangeDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    if (obj.HasMember(shape.USER_NAME)) {
        m_userName = obj[shape.USER_NAME].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.USER_NAME);
        success = false;
    }
    if (obj.HasMember(shape.OLD_PASSWORD)) {
        m_oldPassword = obj[shape.OLD_PASSWORD].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.OLD_PASSWORD);
        success = false;
    }
    if (obj.HasMember(shape.NEW_PASSWORD)) {
        m_newPassword = obj[shape.NEW_PASSWORD].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.NEW_PASSWORD);
        success = false;
    }
    return success;
}
