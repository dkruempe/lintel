#include "base_library/features/property/controller/PropertyValueDto.h"

#include "base_library/core/services/LoggerService.h"

PropertyValueDto::Shapes PropertyValueDto::m_shapes{};

const std::string &PropertyValueDto::getValue() const { return m_value; }

PropertyValueDto::PropertyValueDto(std::string value)
        : m_value(std::move(value)) {}

void PropertyValueDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // value
    writer->String(m_shapes.VALUE.c_str());
    writer->String(m_value.c_str());
    writer->EndObject();
}

bool PropertyValueDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    if (obj.HasMember(m_shapes.VALUE.c_str())) {
        m_value = obj[m_shapes.VALUE.c_str()].GetString();
    } else {
        success = false;
    }
    return success;
}
