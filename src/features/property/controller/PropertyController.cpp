#include "base_library/features/property/controller/PropertyController.h"

#include <utility>

#include "base_library/features/http/service/HttpStatusCodes.h"
#include "base_library/features/property/controller/PropertiesDto.h"
#include "base_library/features/property/controller/PropertyValueDto.h"

PropertyController::PropertyController(
    std::shared_ptr<PropertyService> propertyService,
    const std::shared_ptr<AuthService>& authService)
    : Controller(authService),
      m_propertyService(std::move(propertyService)),
      m_adminGroup("Admin-Property", {}, true),
      m_userGroup("User-Property", {}, true) {
  add(m_adminGroup);
  add(m_userGroup);
}

void PropertyController::allPropertiesOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      PropertiesDto propertiesDto(m_propertyService->allOf());
      response.set_content(propertiesDto.JsonSerializable::serialize() + "\n",
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void PropertyController::allPropertiesOfProcessGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      PropertiesDto propertiesDto(m_propertyService->allOf(processName));
      response.set_content(propertiesDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void PropertyController::allPropertiesOfProcessAndClassGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      PropertiesDto propertiesDto(
          m_propertyService->allOf(processName, className));
      response.set_content(propertiesDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void PropertyController::allPropertiesOfProcessClassAndInstanceGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      PropertiesDto propertiesDto(
          m_propertyService->allOf(processName, className, instanceName));
      response.set_content(propertiesDto.JsonSerializable::serialize(),
                           contentType.getName().c_str());
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void PropertyController::propertyOfGet(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      try {
        auto property = m_propertyService->get(propertyName, instanceName,
                                               className, processName);
        PropertyDto propertyDto(property);
        response.set_content(propertyDto.JsonSerializable::serialize(),
                             contentType.getName().c_str());
      } catch (const PropertyNotFoundException& exception) {
        response.status = HttpStatusCodes::MethodNotAllowed;
        response.set_content("", contentType.getName().c_str());
      }
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
void PropertyController::updatePropertyPut(
    const httplib::Request& request, httplib::Response& response,
    const ContentType& contentType, const std::optional<UserToken>& userToken) {
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
  switch (contentType) {
    case ContentType::ApplicationJson: {
      try {
        PropertyValueDto propertyValueDto;
        propertyValueDto.JsonSerializable::deserialize(request.body);
        auto property = m_propertyService->get(propertyName, instanceName,
                                               className, processName);
        m_propertyService->changeStringValueOf(property,
                                               propertyValueDto.getValue());
      } catch (const PropertyNotFoundException& exception) {
        response.status = HttpStatusCodes::MethodNotAllowed;
        response.set_content("property not found",
                             contentType.getName().c_str());
      } catch (const PropertyNoRuntimeChangeSupported& exception) {
        response.status = HttpStatusCodes::MethodNotAllowed;
        response.set_content("no property runtime change supported",
                             contentType.getName().c_str());
      } catch (const std::exception& e) {
        response.status = HttpStatusCodes::MethodNotAllowed;
        response.set_content("value parameter not found",
                             contentType.getName().c_str());
      }
      break;
    }
    default: {
      response.status = HttpStatusCodes::Forbidden;
      response.set_content("", contentType.getName().c_str());
      break;
    }
  }
}
