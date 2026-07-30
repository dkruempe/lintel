#ifndef CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H
#define CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H

#include <string>

#include "base_library/core/models/JsonSerializable.h"

/** DTO representing a property value update request */
class PropertyValueDto : public JsonSerializable {
private:
    std::string m_value;

    static struct Shapes {
        const std::string VALUE = "value";
    } m_shapes;

public:
    /** @param value the new property value */
    explicit PropertyValueDto(std::string value);

    PropertyValueDto() = default;

    /** @return the value string */
    [[nodiscard]] const std::string &getValue() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYVALUEDTO_H
