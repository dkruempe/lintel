#include "base_library/features/property/controller/PropertyApi.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/property/controller/PropertiesDto.h"
#include "base_library/features/property/controller/PropertyValueDto.h"

std::vector<PropertyDto> PropertyApi::allOf() {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result = m_client->get("/properties", headers);
  if (result->status != HttpStatusCodes::OK) {
    return std::vector<PropertyDto>();
  }
  PropertiesDto propertiesDto;
  try {
    propertiesDto.deserialize(result->body);
  } catch (std::exception& exception) {
    return std::vector<PropertyDto>();
  }
  return propertiesDto.getProperties();
}
std::vector<PropertyDto> PropertyApi::allOf(const std::string& processName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result =
      m_client->get("/properties/" + processName, headers);
  if (result->status != HttpStatusCodes::OK) {
    return std::vector<PropertyDto>();
  }
  PropertiesDto propertiesDto;
  try {
    propertiesDto.deserialize(result->body);
  } catch (std::exception& exception) {
    return std::vector<PropertyDto>();
  }
  return propertiesDto.getProperties();
}
std::vector<PropertyDto> PropertyApi::allOf(const std::string& processName,
                                            const std::string& className) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result =
      m_client->get("/properties/" + processName + "/" + className, headers);
  if (result->status != HttpStatusCodes::OK) {
    return std::vector<PropertyDto>();
  }
  PropertiesDto propertiesDto;
  try {
    propertiesDto.deserialize(result->body);
  } catch (std::exception& exception) {
    return std::vector<PropertyDto>();
  }
  return propertiesDto.getProperties();
}
std::vector<PropertyDto> PropertyApi::allOf(const std::string& processName,
                                            const std::string& className,
                                            const std::string& instanceName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result = m_client->get(
      "/properties/" + processName + "/" + className + "/" + instanceName,
      headers);
  if (result->status != HttpStatusCodes::OK) {
    return std::vector<PropertyDto>();
  }
  PropertiesDto propertiesDto;
  try {
    propertiesDto.deserialize(result->body);
  } catch (std::exception& exception) {
    return std::vector<PropertyDto>();
  }
  return propertiesDto.getProperties();
}
std::optional<PropertyDto> PropertyApi::of(const std::string& processName,
                                           const std::string& className,
                                           const std::string& instanceName,
                                           const std::string& propertyName) {
  httplib::Headers headers{};
  headers.insert({"Content-Type", "application/json"});
  const httplib::Result& result =
      m_client->get("/properties/" + processName + "/" + className + "/" +
                        instanceName + "/" + propertyName,
                    headers);
  if (result->status != HttpStatusCodes::OK) {
    return std::nullopt;
  }
  PropertyDto propertyDto;
  try {
    propertyDto.JsonSerializable::deserialize(result->body);
  } catch (std::exception& exception) {
    return std::nullopt;
  }
  return std::make_optional(propertyDto);
}
bool PropertyApi::updateOf(const PropertyDto& propertyDto,
                           const std::string& value) {
  try {
    PropertyValueDto propertyValueDto(value);
    const httplib::Result& result = m_client->put(
        "/properties/" + propertyDto.getProcessName() + "/" +
            propertyDto.getClassName() + "/" + propertyDto.getInstanceName() +
            "/" + propertyDto.getName(),
        propertyValueDto.JsonSerializable::serialize(), "application/json");
    if (result->status != HttpStatusCodes::OK) {
      LOG_ERROR("property {} update value no success {}:{}",
                propertyDto.getName(), result->status, result->body);
    }
  } catch (std::exception& e) {
    LOG_ERROR("exception during update of property {} with {}: {}",
              propertyDto.getName(), value, e.what());
    return false;
  }
  return true;
}
PropertyApi::PropertyApi(const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
