#ifndef CPP_BASE_LIBRARY_PROPERTIESDTO_H
#define CPP_BASE_LIBRARY_PROPERTIESDTO_H

#include <memory>
#include <vector>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/property/controller/PropertyDto.h"
#include "base_library/features/property/models/PropertyBase.h"

class PropertiesDto : public JsonSerializable {
private:
    std::vector<PropertyDto> m_properties;

    static std::vector<PropertyDto> build(
            const std::vector<std::shared_ptr<PropertyBase>> &properties);

public:
    explicit PropertiesDto(
            const std::vector<std::shared_ptr<PropertyBase>> &properties);

    PropertiesDto() = default;

    [[nodiscard]] const std::vector<PropertyDto> &getProperties() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    void deserialize(const std::string &json) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTIESDTO_H
