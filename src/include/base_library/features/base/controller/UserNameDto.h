#ifndef CPP_BASE_LIBRARY_USERNAMEDTO_H
#define CPP_BASE_LIBRARY_USERNAMEDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/services/AuthService.h"

/** DTO representing a user name */
class UserNameDto : public JsonSerializable {
private:
    /** The user name */
    std::string m_userName;

    /** JSON field name constants */
    static struct Shapes {
        const char* const USER_NAME = "user_name";
    } shape;

public:
    /** Construct from a User model
     * @param user The source user */
    explicit UserNameDto(const User &user);

    /** Construct from a user name string
     * @param userName The user name */
    explicit UserNameDto(std::string userName);

    /** Default constructor */
    UserNameDto() = default;

    /** Get the user name
     * @return The user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

/** DTO representing a collection of user names */
class UserNamesDto : public JsonSerializable {
private:
    /** The user name DTOs */
    std::vector<UserNameDto> m_userNames;

    /** Initialize from User models
     * @param users The source users
     * @return Vector of user name DTOs */
    static std::vector<UserNameDto> init(const std::vector<User> &users);

    /** Initialize from string vectors
     * @param users The source user name strings
     * @return Vector of user name DTOs */
    static std::vector<UserNameDto> init(const std::vector<std::string> &users);

public:
    /** Construct from User models
     * @param users The source users */
    explicit UserNamesDto(const std::vector<User> &users);

    /** Construct from user name strings
     * @param userNames The source user name strings */
    explicit UserNamesDto(const std::vector<std::string> &userNames);

    /** Default constructor */
    UserNamesDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;

    /** Get the user name DTOs
     * @return Vector of user name DTOs */
    [[nodiscard]] const std::vector<UserNameDto> &getUserNames() const;
};

#endif  // CPP_BASE_LIBRARY_USERNAMEDTO_H
