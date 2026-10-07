#ifndef LINTEL_PROPERTYCONTROLLER_H
#define LINTEL_PROPERTYCONTROLLER_H

#include "lintel/features/base/services/IHistoryService.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/Controller.h"
#include "lintel/features/property/services/PropertyService.h"

/** HTTP controller for property query and update operations */
class PropertyController : public Controller {
private:
    std::shared_ptr<PropertyService> m_propertyService;
    std::shared_ptr<IHistoryService> m_historyService;
    Group m_adminGroup;
    Group m_userGroup;
    ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+))", Get,
                       allPropertiesOf);

    ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+)/([^\/]+))", Get,
                       propertyOf);

    ADD_HANDLER_METHOD(R"(/properties/([^\/]+)/([^\/]+)/([^\/]+)/([^\/]+))", Put,
                       updateProperty);

public:
    explicit PropertyController(std::shared_ptr<PropertyService> propertyService,
                                const std::shared_ptr<IAuthService> &authService,
                                std::shared_ptr<IHistoryService> historyService);
};

#endif  // LINTEL_PROPERTYCONTROLLER_H
