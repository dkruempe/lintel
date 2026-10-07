#ifndef LINTEL_USERLOGINDTO_H
#define LINTEL_USERLOGINDTO_H

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/services/AuthService.h"

/** DTO for user login credentials */
class UserLoginDto : public JsonSerializable {
private:
    /** The user name */
    std::string m_userName;
    /** The password */
    std::string m_password;

    /** JSON field name constants */
    static struct Shapes {
        const char* const USER_NAME = "user_name";
        const char* const PASSWORD = "password";
    } shape;

public:
    /** Construct from a UserLogin model
     * @param userLogin The source user login */
    explicit UserLoginDto(const UserLogin &userLogin);

    /** Construct from user name and password
     * @param userName The user name
     * @param password The password */
    UserLoginDto(std::string userName, std::string password);

    /** Default constructor */
    UserLoginDto() = default;

    /** Get the user name
     * @return The user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Get the password
     * @return The password */
    [[nodiscard]] const std::string &getPassword() const;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // LINTEL_USERLOGINDTO_H
