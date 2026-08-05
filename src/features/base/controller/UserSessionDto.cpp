#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/controller/UserSessionDto.h"

#include "base_library/core/services/LoggerService.h"
import base_library.core.utils;

UserSessionDto::Shapes UserSessionDto::shape{};

UserSessionDto::UserSessionDto(const UserToken &userToken)
        : m_id(userToken.m_id),
          m_ipAddress(userToken.m_ipAddress),
          m_userName(userToken.m_user.getUserName()),
          m_lastAccessTimestamps(userToken.m_lastAccessTimestamps) {}

const std::string &UserSessionDto::getId() const { return m_id; }

const std::string &UserSessionDto::getIpAddress() const { return m_ipAddress; }

const std::string &UserSessionDto::getUserName() const { return m_userName; }

const date::sys_time<std::chrono::microseconds> &
UserSessionDto::getLastAccessTimestamps() const {
    return m_lastAccessTimestamps;
}

std::vector<UserSessionDto> UserSessionDto::listOf(
        const std::vector<UserToken> &tokens) {
    std::vector<UserSessionDto> result;
    result.reserve(tokens.size());
    for (const auto &token: tokens) {
        result.emplace_back(token);
    }
    return result;
}

void UserSessionDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // ID
    writer->String(shape.ID);
    writer->String(m_id.c_str());
    // IP_ADDRESS
    writer->String(shape.IP_ADDRESS);
    writer->String(m_ipAddress.c_str());
    // USER_NAME
    writer->String(shape.USER_NAME);
    writer->String(m_userName.c_str());
    // LAST_ACCESS
    writer->String(shape.LAST_ACCESS);
    writer->String(
            StringifyService<date::sys_time<std::chrono::microseconds>>::
                    serializeToString(m_lastAccessTimestamps)
                            .c_str());
    writer->EndObject();
}

bool UserSessionDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    // ID
    if (obj.HasMember(shape.ID)) {
        m_id = obj[shape.ID].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.ID);
        success = false;
    }
    // IP_ADDRESS
    if (obj.HasMember(shape.IP_ADDRESS)) {
        m_ipAddress = obj[shape.IP_ADDRESS].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.IP_ADDRESS);
        success = false;
    }
    // USER_NAME
    if (obj.HasMember(shape.USER_NAME)) {
        m_userName = obj[shape.USER_NAME].GetString();
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.USER_NAME);
        success = false;
    }
    // LAST_ACCESS
    if (obj.HasMember(shape.LAST_ACCESS)) {
        m_lastAccessTimestamps =
                StringifyService<date::sys_time<std::chrono::microseconds>>::
                deserializeFromString(obj[shape.LAST_ACCESS].GetString());
    } else {
        LOG_ERROR("{} not defined in json serialization", shape.LAST_ACCESS);
        success = false;
    }
    return success;
}
