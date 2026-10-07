#include <httplib.h>

#include "lintel/features/property/controller/PropertyApi.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/controller/PropertyValueDto.h"

std::vector<PropertyDto> PropertyApi::allOf(const std::string &processName,
                                            const std::string &className,
                                            const std::string &instanceName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    const httplib::Result &result = m_client->get(
            "/properties/" + processName + "/" + className + "/" + instanceName,
            headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return {};
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            // all fine => no handling needed
            break;
        default:
            // currently no extra handling
            return {};
    }
    PropertiesDto propertiesDto;
    try {
        propertiesDto.deserialize(result->body);
    } catch (const std::exception &exception) {
        return {};
    }
    return propertiesDto.getProperties();
}

std::optional<PropertyDto> PropertyApi::of(const std::string &processName,
                                           const std::string &className,
                                           const std::string &instanceName,
                                           const std::string &propertyName) {
    httplib::Headers headers{};
    headers.insert({"Content-Type", "application/json"});
    const httplib::Result &result =
            m_client->get("/properties/" + processName + "/" + className + "/" +
                          instanceName + "/" + propertyName,
                          headers);
    if (!HttpClientHelper::hasResponse(result)) {
        return std::nullopt;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            // all fine => no handling needed
            break;
        default:
            // currently no extra handling
            return std::nullopt;
    }
    PropertyDto propertyDto;
    try {
        propertyDto.JsonSerializable::deserialize(result->body);
    } catch (const std::exception &exception) {
        return std::nullopt;
    }
    return std::make_optional(propertyDto);
}

bool PropertyApi::updateOf(const PropertyDto &propertyDto,
                           const std::string &value) {
    PropertyValueDto propertyValueDto(value);
    const httplib::Result &result = m_client->put(
            "/properties/" + propertyDto.getProcessName() + "/" +
            propertyDto.getClassName() + "/" + propertyDto.getInstanceName() +
            "/" + propertyDto.getName(),
            propertyValueDto.JsonSerializable::serialize(), "application/json");
    if (!HttpClientHelper::hasResponse(result)) {
        return false;
    }
    HttpStatusCodes status(result->status);
    switch (status) {
        case HttpStatusCodes::Unauthorized:
            throw HttpUnauthorizedException();
        case HttpStatusCodes::OK:
            // all fine => no handling needed
            break;
        default:
            LOG_ERROR("property {} update value no success {}:{}",
                      propertyDto.getName(), result->status, result->body);
            // currently no extra handling
            return false;
    }
    return true;
}

PropertyApi::PropertyApi(const std::shared_ptr<ClientProvider> &clientProvider)
        : m_client(clientProvider->provide()) {}
