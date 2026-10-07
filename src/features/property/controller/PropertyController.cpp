#include "lintel/features/property/controller/PropertyController.h"
#include "lintel/core/models/JsonSerializable.h"
#include <fmt/core.h>

#include <optional>
#include <utility>

#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/controller/PropertyValueDto.h"

PropertyController::PropertyController(
        std::shared_ptr<PropertyService> propertyService,
        const std::shared_ptr<IAuthService> &authService,
        std::shared_ptr<IHistoryService> historyService)
        : Controller(authService),
          m_propertyService(std::move(propertyService)),
          m_historyService(std::move(historyService)),
          m_adminGroup("Admin-Property", {}, true),
          m_userGroup("User-Property", {}, true) {
    add(m_adminGroup);
    add(m_userGroup);
}

void PropertyController::allPropertiesOfGet(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &userToken) {
    // no user logged in => Unauthorized
    if (!userToken.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    if (!userToken->m_user.has(m_userGroup) &&
        !userToken->m_user.has(m_adminGroup)) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName());
        return;
    }
    const std::string processName = request.matches[1];
    const std::string className = request.matches[2];
    const std::string instanceName = request.matches[3];
    if (contentType == ContentType::ApplicationJson) {
        PropertiesDto propertiesDto(
                m_propertyService->allOf(processName, className, instanceName));
        response.set_content(propertiesDto.JsonSerializable::serialize(),
                             contentType.getName());
    } else {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName());
    }
}

void PropertyController::propertyOfGet(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &userToken) {
    // no user logged in => Unauthorized
    if (!userToken.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName().c_str());
        return;
    }
    if (!userToken->m_user.has(m_userGroup) &&
        !userToken->m_user.has(m_adminGroup)) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName().c_str());
        return;
    }
    const std::string processName = request.matches[1];
    const std::string className = request.matches[2];
    const std::string instanceName = request.matches[3];
    const std::string propertyName = request.matches[4];
    if (contentType == ContentType::ApplicationJson) {
        try {
            auto property = m_propertyService->get(propertyName, instanceName,
                                                   className, processName);
            PropertyDto propertyDto(property);
            response.set_content(propertyDto.JsonSerializable::serialize(),
                                 contentType.getName());
        } catch (const PropertyNotFoundException &exception) {
            response.status = HttpStatusCodes::MethodNotAllowed;
            response.set_content("", contentType.getName());
        }
    } else {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName());
    }
}

void PropertyController::updatePropertyPut(
        const httplib::Request &request, httplib::Response &response,
        const ContentType &contentType, const std::optional<UserToken> &userToken) {
    // no user logged in => Unauthorized
    if (!userToken.has_value()) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName().c_str());
        return;
    }
    if (!userToken->m_user.has(m_adminGroup)) {
        response.status = HttpStatusCodes::Unauthorized;
        response.set_content("", contentType.getName().c_str());
        return;
    }
    const std::string processName = request.matches[1];
    const std::string className = request.matches[2];
    const std::string instanceName = request.matches[3];
    const std::string propertyName = request.matches[4];
    if (contentType == ContentType::ApplicationJson) {
        try {
            PropertyValueDto propertyValueDto;
            propertyValueDto.JsonSerializable::deserialize(request.body);
            auto property = m_propertyService->get(propertyName, instanceName,
                                                   className, processName);
            std::string oldValue = property->toString();
            std::string newValue = propertyValueDto.getValue();
            m_propertyService->changeStringValueOf(property,
                                                   propertyValueDto.getValue());
            DEFINE_HISTORY_ENTRY2(historyEntry, "PROPERTY",
                                  fmt::format("User: {}, changed {} -> {}", userToken->m_user.getUserName(),
                                              oldValue, newValue),
                                  m_historyService->getProcessName(),
                                  "PropertyController");
            m_historyService->historizeOf({historyEntry});
        } catch (const PropertyNotFoundException &exception) {
            response.status = HttpStatusCodes::MethodNotAllowed;
            response.set_content("property not found",
                                 contentType.getName());
        } catch (const PropertyNoRuntimeChangeSupported &exception) {
            response.status = HttpStatusCodes::MethodNotAllowed;
            response.set_content("no property runtime change supported",
                                 contentType.getName());
        } catch (const std::exception &e) {
            response.status = HttpStatusCodes::MethodNotAllowed;
            response.set_content("value parameter not found",
                                 contentType.getName());
        }
    } else {
        response.status = HttpStatusCodes::Forbidden;
        response.set_content("", contentType.getName());
    }
}
