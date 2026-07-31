#ifndef CPP_BASE_LIBRARY_USERSESSIONSDTO_H
#define CPP_BASE_LIBRARY_USERSESSIONSDTO_H

#include <utility>
#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/UserSessionDto.h"

/** DTO representing a collection of active user sessions. */
class UserSessionsDto : public JsonSerializable {
private:
    /** The session DTOs */
    std::vector<UserSessionDto> m_sessions;

public:
    /** Construct from existing session DTOs
     * @param sessions the session DTOs */
    explicit UserSessionsDto(std::vector<UserSessionDto> sessions);

    /** Default constructor */
    UserSessionsDto() = default;

    /** Get the session DTOs
     * @return vector of session DTOs */
    [[nodiscard]] const std::vector<UserSessionDto> &getSessions() const;

    /** Serialize to JSON
     * @param writer the rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj the JSON value
     * @return true on success */
    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERSESSIONSDTO_H
