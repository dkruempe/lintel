#include "base_library/features/property/controller/PropertiesDto.h"
const std::vector<PropertyDto>& PropertiesDto::getProperties() const {
  return m_properties;
}
std::vector<PropertyDto> PropertiesDto::build(
    const std::vector<std::shared_ptr<PropertyBase>>& properties) {
  std::vector<PropertyDto> propertiesDto;
  propertiesDto.reserve(properties.size());
  for (const auto& property : properties) {
    propertiesDto.emplace_back(property);
  }
  return propertiesDto;
}
PropertiesDto::PropertiesDto(
    const std::vector<std::shared_ptr<PropertyBase>>& properties)
    : m_properties(build(properties)) {}
void PropertiesDto::serialize(
    rapidjson::Writer<rapidjson::StringBuffer>* writer) const {
  writer->StartArray();
  for (const auto& property : m_properties) {
    property.serialize(writer);
  }
  writer->EndArray();
}
bool PropertiesDto::deserialize(const rapidjson::Value& obj) {
  if (obj.IsArray()) {
    return false;
  }

  bool success = true;
  for (const auto& property : obj.GetArray()) {
    PropertyDto propertyDto;
    success = propertyDto.deserialize(property);
    if (!success) {
      break;
    }
    m_properties.push_back(propertyDto);
  }
  return success;
}
