#ifndef CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H
#define CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H

#include "base_library/features/http/service/Controller.h"
#include "base_library/features/property/services/PropertyService.h"

class PropertyController : public Controller {
 private:
  std::shared_ptr<PropertyService> m_propertyService;
  ADD_HANDLER_METHOD("/properties", Get, allPropertiesOf);
  ADD_HANDLER_METHOD(R"(/properties/(\w+))", Get, allPropertiesOfProcess);
  ADD_HANDLER_METHOD(R"(/properties/(\w+)/(\w+))", Get,
                     allPropertiesOfProcessAndClass);
  ADD_HANDLER_METHOD(R"(/properties/(\w+)/(\w+)/(\w+))", Get,
                     allPropertiesOfProcessClassAndInstance);
  ADD_HANDLER_METHOD(R"(/properties/(\w+)/(\w+)/(\w+)/(\w+))", Get,
                     propertyOf);
  ADD_HANDLER_METHOD(R"(/properties/(\w+)/(\w+)/(\w+)/(\w+))", Put,
                     updateProperty);

 public:
  explicit PropertyController(std::shared_ptr<PropertyService> propertyService);
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCONTROLLER_H
