#include "lintel/features/base/controller/UserSessionsDto.h"

UserSessionsDto::UserSessionsDto(std::vector<UserSessionDto> sessions)
        : m_sessions(std::move(sessions)) {}

const std::vector<UserSessionDto> &UserSessionsDto::getSessions() const {
    return m_sessions;
}

void UserSessionsDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartArray();
    for (const auto &session: m_sessions) {
        session.serialize(writer);
    }
    writer->EndArray();
}

bool UserSessionsDto::deserialize(const rapidjson::Value &obj) {
    if (!obj.IsArray()) {
        return false;
    }
    for (auto iter = obj.Begin(); iter != obj.End(); iter++) {
        UserSessionDto sessionDto;
        sessionDto.deserialize(*iter);
        m_sessions.push_back(std::move(sessionDto));
    }
    return true;
}
