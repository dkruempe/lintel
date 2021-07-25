#include "base_library/features/property/controller/PropertyController.h"

#include "base_library/features/property/controller/PropertiesDto.h"

PropertyController::PropertyController(
    std::shared_ptr<PropertyService> propertyService)
    : Controller(), m_propertyService(std::move(propertyService)) {}
void PropertyController::allPropertiesOfGet(const httplib::Request& request,
                                            httplib::Response& response) {
  PropertiesDto propertiesDto(m_propertyService->allOf());
  response.set_content(propertiesDto.JsonSerializable::serialize() + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  PropertiesDto propertiesDto(m_propertyService->allOf(processName));
  response.set_content(propertiesDto.JsonSerializable::serialize() + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessAndClassGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  PropertiesDto propertiesDto(m_propertyService->allOf(processName, className));
  response.set_content(propertiesDto.JsonSerializable::serialize() + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessClassAndInstanceGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  const std::string instanceName = request.matches[3];
  PropertiesDto propertiesDto(
      m_propertyService->allOf(processName, className, instanceName));
  response.set_content(propertiesDto.JsonSerializable::serialize() + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessClassInstanceAndNameGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  const std::string instanceName = request.matches[3];
  const std::string propertyName = request.matches[4];
  auto property = m_propertyService->get(propertyName, instanceName, className,
                                         processName);
  if (property != nullptr) {
    PropertyDto propertyDto(property);
    response.set_content(propertyDto.JsonSerializable::serialize() + "\n",
                         "text/plain");
  } else {
    response.set_content("no property found \n", "text/plain");
  }
}
