#ifndef LINTEL_PROPERTIESDTO_H
#define LINTEL_PROPERTIESDTO_H

#include <memory>
#include <string>
#include <vector>

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/property/controller/PropertyDto.h"
#include "lintel/features/property/models/PropertyBase.h"

/** DTO for a collection of properties, JSON-serializable */
class PropertiesDto : public JsonSerializable {
private:
    std::vector<PropertyDto> m_properties;

    /** Build DTO list from property base shared pointers */
    static std::vector<PropertyDto> build(
            const std::vector<std::shared_ptr<PropertyBase>> &properties);

public:
    /** @param properties list of property bases to convert */
    explicit PropertiesDto(
            const std::vector<std::shared_ptr<PropertyBase>> &properties);

    PropertiesDto() = default;

    /** @return the list of property DTOs */
    [[nodiscard]] const std::vector<PropertyDto> &getProperties() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    void deserialize(const std::string &json) override;
};

#endif  // LINTEL_PROPERTIESDTO_H
