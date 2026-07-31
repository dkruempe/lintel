#ifndef CPP_BASE_LIBRARY_USERSESSIONDTO_H
#define CPP_BASE_LIBRARY_USERSESSIONDTO_H

#include <date/date.h>

#include <chrono>
#include <string>
#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/services/IAuthService.h"

/** DTO representing an active user session. */
class UserSessionDto : public JsonSerializable {
private:
    /** The session/token id */
    std::string m_id;
    /** The IP address the session was created from */
    std::string m_ipAddress;
    /** The user name the session belongs to */
    std::string m_userName;
    /** The last access timestamp */
    date::sys_time<std::chrono::microseconds> m_lastAccessTimestamps;

    /** JSON field name constants */
    static struct Shapes {
        const char *const ID = "id";
        const char *const IP_ADDRESS = "ip_address";
        const char *const USER_NAME = "user_name";
        const char *const LAST_ACCESS = "last_access";
    } shape;

public:
    /** Construct from a UserToken model
     * @param userToken the source session */
    explicit UserSessionDto(const UserToken &userToken);

    /**
     * Build session DTOs from a list of user tokens.
     * @param tokens the source sessions
     * @return list of session DTOs
     */
    static std::vector<UserSessionDto> listOf(const std::vector<UserToken> &tokens);

    /** Default constructor */
    UserSessionDto() = default;

    /** Get the session id
     * @return the session id */
    [[nodiscard]] const std::string &getId() const;

    /** Get the IP address
     * @return the IP address */
    [[nodiscard]] const std::string &getIpAddress() const;

    /** Get the user name
     * @return the user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Get the last access timestamp
     * @return the last access timestamp */
    [[nodiscard]] const date::sys_time<std::chrono::microseconds>
    &getLastAccessTimestamps() const;

    /** Serialize to JSON
     * @param writer the rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj the JSON value
     * @return true on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERSESSIONDTO_H
