#ifndef CPP_BASE_LIBRARY_USERDTO_H
#define CPP_BASE_LIBRARY_USERDTO_H

#include <memory>
#include <optional>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/GroupDto.h"
#include "base_library/features/base/models/User.h"

/** DTO representing a user */
class UserDto : public JsonSerializable {
private:
    /** The first name */
    std::string m_firstName;
    /** The last name */
    std::string m_lastName;
    /** The email address */
    std::string m_eMail;
    /** The user name */
    std::string m_userName;
    /** The user ID */
    std::string m_id;
    /** The sex */
    User::Sex m_sex = User::Sex::Male;
    /** The password (optional, used for creation only) */
    std::optional<std::string> m_password = std::nullopt;
    /** The account creation timestamp */
    date::sys_time<std::chrono::microseconds> m_createdTimestamp;
    /** The groups this user belongs to */
    std::shared_ptr<GroupsDto> m_groups;

    /** JSON field name constants */
    static struct Shapes {
        const char *const FIRST_NAME = "first_name";
        const char *const LAST_NAME = "last_name";
        const char *const EMAIL = "email";
        const char *const USER_NAME = "user_name";
        const char *const SEX = "sex";
        const char *const CREATED_TIMESTAMP = "created_timestamp";
        const char *const GROUPS = "groups";
        const char *const ID = "id";
        const char *const PASSWORD = "password";
    } shape;

public:
    /** Get the first name
     * @return The first name */
    [[nodiscard]] const std::string &getFirstName() const;

    /** Get the last name
     * @return The last name */
    [[nodiscard]] const std::string &getLastName() const;

    /** Get the email address
     * @return The email */
    [[nodiscard]] const std::string &getEMail() const;

    /** Get the user name
     * @return The user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Get the user ID
     * @return The ID */
    [[nodiscard]] const std::string &getId() const;

    /** Get the sex
     * @return The sex */
    [[nodiscard]] User::Sex getSex() const;

    /** Get the groups this user belongs to
     * @return Vector of group DTOs */
    [[nodiscard]] std::vector<GroupDto> getGroups() const;

    /** Get the account creation timestamp
     * @return The timestamp */
    [[nodiscard]] const date::sys_time<std::chrono::microseconds>
    &getCreatedTimestamp() const;

    /** Get the password (only used for creating a user)
     * @return Optional password string */
    [[nodiscard]] const std::optional<std::string> &getPassword() const;

    /** Set the password
     * @param password The password to set */
    void setPassword(const std::string &password);

    /** Construct from a User model
     * @param user The source user
     * @param id Optional user ID */
    explicit UserDto(const User &user, std::string id = "");

    /** Default constructor */
    UserDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

/** DTO representing a collection of users */
class UsersDto : public JsonSerializable {
private:
    /** The user DTOs */
    std::vector<UserDto> m_users;

    /** Initialize user DTOs from User models
     * @param users The source users
     * @return Vector of user DTOs */
    static std::vector<UserDto> init(const std::vector<User> &users);

public:
    /** Construct from User models
     * @param users The source users */
    explicit UsersDto(const std::vector<User> &users);

    /** Default constructor */
    UsersDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;

    /** Get the user DTOs
     * @return Vector of user DTOs */
    [[nodiscard]] const std::vector<UserDto> &getUsers() const;
};

#endif  // CPP_BASE_LIBRARY_USERDTO_H
