#include "lintel/features/base/controller/UserLoginDto.h"

UserLoginDto::Shapes UserLoginDto::shape{};

UserLoginDto::UserLoginDto(const UserLogin &userLogin)
        : m_userName(userLogin.m_userName), m_password(userLogin.m_password) {}

UserLoginDto::UserLoginDto(std::string userName, std::string password)
        : m_userName(std::move(userName)), m_password(std::move(password)) {}

const std::string &UserLoginDto::getUserName() const { return m_userName; }

const std::string &UserLoginDto::getPassword() const { return m_password; }

void UserLoginDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // USERNAME
    writer->String(shape.USER_NAME);
    writer->String(m_userName.c_str());
    // PASSWORD
    writer->String(shape.PASSWORD);
    writer->String(m_password.c_str());
    writer->EndObject();
}

bool UserLoginDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    // USERNAME
    if (obj.HasMember(shape.USER_NAME)) {
        m_userName = obj[shape.USER_NAME].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", shape.USER_NAME);
    }
    // PASSWORD
    if (obj.HasMember(shape.PASSWORD)) {
        m_password = obj[shape.PASSWORD].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", shape.PASSWORD);
    }
    return success;
}