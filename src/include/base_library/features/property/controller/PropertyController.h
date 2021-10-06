#ifndef CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H
#define CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H

#include "base_library/features/http/service/Controller.h"
#include "base_library/features/property/services/PropertyService.h"

class PropertyController : public Controller {
 private:
  std::shared_ptr<PropertyService> m_propertyService;
  Group m_adminGroup;
  Group m_userGroup;
  ADD_HANDLER_METHOD("/properties", Get, allPropertiesOf);
  ADD_HANDLER_METHOD(R"(/properties/([^\/]+))", Get, allPropertiesOfProcess);
  ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+))", Get,
                     allPropertiesOfProcessAndClass);
  ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+))", Get,
                     allPropertiesOfProcessClassAndInstance);
  ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+)/([^\/]+))", Get,
                     propertyOf);
  ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+)/([^\/]+))", Put,
                     updateProperty);

 public:
  explicit PropertyController(std::shared_ptr<PropertyService> propertyService,
                              const std::shared_ptr<AuthService> &authService);
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H
