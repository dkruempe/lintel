#include "lintel/features/base/controller/UserGroupDto.h"

#include "lintel/core/services/LoggerService.h"

UserGroupDto::Shapes UserGroupDto::shape{};

UserGroupDto::UserGroupDto(std::set<std::string> groupAdds,
                           std::set<std::string> groupRemoves,
                           std::string userName)
        : m_groupAdds(std::move(groupAdds)),
          m_groupRemoves(std::move(groupRemoves)),
          m_userName(std::move(userName)) {}

const std::string &UserGroupDto::getUserName() { return m_userName; }

const std::set<std::string> &UserGroupDto::getGroupAdds() {
    return m_groupAdds;
}

const std::set<std::string> &UserGroupDto::getGroupRemoves() {
    return m_groupRemoves;
}

void UserGroupDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    writer->String(shape.USER_NAME);
    writer->String(m_userName.c_str());
    writer->String(shape.GROUPS_ADD);
    writer->StartArray();
    for (const auto &item: m_groupAdds) {
        writer->String(item.c_str());
    }
    writer->EndArray();
    writer->String(shape.GROUPS_REMOVE);
    writer->StartArray();
    for (const auto &item: m_groupRemoves) {
        writer->String(item.c_str());
    }
    writer->EndArray();
    writer->EndObject();
}

bool UserGroupDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    if (obj.HasMember(shape.USER_NAME)) {
        m_userName = obj[shape.USER_NAME].GetString();
    } else {
        LOG_ERROR("failed to deserialize user_name");
        success = false;
    }
    if (obj.HasMember(shape.GROUPS_ADD) &&
        obj[shape.GROUPS_ADD].IsArray()) {
        for (auto iter = obj[shape.GROUPS_ADD].Begin();
             iter != obj[shape.GROUPS_ADD].End(); iter++) {
            m_groupAdds.insert(iter->GetString());
        }
    } else {
        LOG_ERROR("failed to deserialize user_name");
        success = false;
    }
    if (obj.HasMember(shape.GROUPS_REMOVE) &&
        obj[shape.GROUPS_REMOVE].IsArray()) {
        for (auto iter = obj[shape.GROUPS_REMOVE].Begin();
             iter != obj[shape.GROUPS_REMOVE].End(); iter++) {
            m_groupRemoves.insert(iter->GetString());
        }
    } else {
        LOG_ERROR("failed to deserialize user_name");
        success = false;
    }
    return success;
}
