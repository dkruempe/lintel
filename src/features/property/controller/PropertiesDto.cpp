#include "base_library/features/property/controller/PropertiesDto.h"
#include <rapidjson/document.h>

const std::vector<PropertyDto> &PropertiesDto::getProperties() const { return m_properties; }

std::vector<PropertyDto> PropertiesDto::build(
  const std::vector<std::shared_ptr<PropertyBase> > &properties)
{
  std::vector<PropertyDto> propertiesDto;
  propertiesDto.reserve(properties.size());
  std::transform(properties.begin(),
    properties.end(),
    std::back_inserter(propertiesDto),
    [](const std::shared_ptr<PropertyBase> &property) { return PropertyDto(property); });
  return propertiesDto;
}

PropertiesDto::PropertiesDto(
  const std::vector<std::shared_ptr<PropertyBase> > &properties)
  : m_properties(build(properties)) {}

void PropertiesDto::serialize(
  rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &property : m_properties) { property.serialize(writer); }
  writer->EndArray();
}

void PropertiesDto::deserialize(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (!document.IsArray()) { return; }
  for (const auto &property : document.GetArray()) {
    PropertyDto propertyDto;
    propertyDto.deserialize(property);
    m_properties.push_back(propertyDto);
  }
}