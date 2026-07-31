#ifndef CPP_BASE_LIBRARY_USERPASSWORDCHANGEDTO_H
#define CPP_BASE_LIBRARY_USERPASSWORDCHANGEDTO_H

#include <string>

#include "base_library/core/models/JsonSerializable.h"

/** DTO for changing a user's password (passwords are Base64-encoded). */
class UserPasswordChangeDto : public JsonSerializable {
private:
    /** The target user name */
    std::string m_userName;
    /** The current password (Base64-encoded) */
    std::string m_oldPassword;
    /** The new password (Base64-encoded) */
    std::string m_newPassword;

    /** JSON field name constants */
    static struct Shapes {
        const char *const USER_NAME = "user_name";
        const char *const OLD_PASSWORD = "old_password";
        const char *const NEW_PASSWORD = "new_password";
    } shape;

public:
    /** Default constructor */
    UserPasswordChangeDto() = default;

    /** Construct from all fields
     * @param userName the target user name
     * @param oldPassword the current password (Base64-encoded)
     * @param newPassword the new password (Base64-encoded) */
    UserPasswordChangeDto(std::string userName, std::string oldPassword,
                          std::string newPassword);

    /** Get the target user name
     * @return the user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Get the current password
     * @return the current password */
    [[nodiscard]] const std::string &getOldPassword() const;

    /** Get the new password
     * @return the new password */
    [[nodiscard]] const std::string &getNewPassword() const;

    /** Serialize to JSON
     * @param writer the rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj the JSON value
     * @return true on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERPASSWORDCHANGEDTO_H
