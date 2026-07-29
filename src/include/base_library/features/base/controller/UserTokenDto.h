#ifndef CPP_BASE_LIBRARY_USERTOKENDTO_H
#define CPP_BASE_LIBRARY_USERTOKENDTO_H

#include "base_library/core/models/JsonSerializable.h"

class UserTokenDto : public JsonSerializable {
private:
    std::string m_id;

    static struct Shapes {
        const char* const ID = "id";
    } shape;

public:
    explicit UserTokenDto(std::string id);

    UserTokenDto() = default;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;

    const std::string &getId() const;
};

#endif  // CPP_BASE_LIBRARY_USERTOKENDTO_H
