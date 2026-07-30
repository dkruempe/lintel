#ifndef CPP_BASE_LIBRARY_USERTOKENDTO_H
#define CPP_BASE_LIBRARY_USERTOKENDTO_H

#include "base_library/core/models/JsonSerializable.h"

/** DTO representing a user token */
class UserTokenDto : public JsonSerializable {
private:
    /** The token ID */
    std::string m_id;

    /** JSON field name constants */
    static struct Shapes {
        const char* const ID = "id";
    } shape;

public:
    /** Construct from a token ID string
     * @param id The token ID */
    explicit UserTokenDto(std::string id);

    /** Default constructor */
    UserTokenDto() = default;

    /** Serialize to JSON
     * @param writer The rapidjson writer */
    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    /** Deserialize from JSON
     * @param obj The JSON value
     * @return True on success */
    bool deserialize(const rapidjson::Value &obj) override;

    /** Get the token ID
     * @return The ID */
    const std::string &getId() const;
};

#endif  // CPP_BASE_LIBRARY_USERTOKENDTO_H
