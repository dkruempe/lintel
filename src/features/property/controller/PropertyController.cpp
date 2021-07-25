#include "base_library/features/property/controller/PropertyController.h"

#include "base_library/features/property/controller/PropertiesDto.h"

PropertyController::PropertyController(
    std::shared_ptr<PropertyService> propertyService)
    : Controller(), m_propertyService(std::move(propertyService)) {}
void PropertyController::allPropertiesOfGet(const httplib::Request& request,
                                            httplib::Response& response) {
  PropertiesDto propertiesDto(m_propertyService->allProperties());
  response.set_content( propertiesDto.JsonSerializable::serialize() + "\n", "text/plain");
}
void PropertyController::allPropertiesOfProcessGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  response.set_content("received request with param: " + processName + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessAndClassGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  response.set_content(
      "received request with param: " + processName + ", " + className + "\n",
      "text/plain");
}
void PropertyController::allPropertiesOfProcessClassAndInstanceGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  const std::string instanceName = request.matches[3];
  response.set_content("received request with param: " + processName + ", " +
                           className + ", " + instanceName + "\n",
                       "text/plain");
}
void PropertyController::allPropertiesOfProcessClassInstanceAndNameGet(
    const httplib::Request& request, httplib::Response& response) {
  const std::string processName = request.matches[1];
  const std::string className = request.matches[2];
  const std::string instanceName = request.matches[3];
  const std::string propertyName = request.matches[4];
  response.set_content("received request with param: " + processName + ", " +
                           className + ", " + instanceName + ", " +
                           propertyName + "\n",
                       "text/plain");
}
