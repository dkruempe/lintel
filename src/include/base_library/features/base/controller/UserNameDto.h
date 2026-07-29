#ifndef CPP_BASE_LIBRARY_USERNAMEDTO_H
#define CPP_BASE_LIBRARY_USERNAMEDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/services/AuthService.h"

class UserNameDto : public JsonSerializable {
private:
    std::string m_userName;

    static struct Shapes {
        const char* const USER_NAME = "user_name";
    } shape;

public:
    explicit UserNameDto(const User &user);

    explicit UserNameDto(std::string userName);

    UserNameDto() = default;

    [[nodiscard]] const std::string &getUserName() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

class UserNamesDto : public JsonSerializable {
private:
    std::vector<UserNameDto> m_userNames;

    static std::vector<UserNameDto> init(const std::vector<User> &users);

    static std::vector<UserNameDto> init(const std::vector<std::string> &users);

public:
    explicit UserNamesDto(const std::vector<User> &users);

    explicit UserNamesDto(const std::vector<std::string> &userNames);

    UserNamesDto() = default;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;

    [[nodiscard]] const std::vector<UserNameDto> &getUserNames() const;
};

#endif  // CPP_BASE_LIBRARY_USERNAMEDTO_H
