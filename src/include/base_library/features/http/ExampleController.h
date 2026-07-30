#ifndef CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H
#define CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H

#include "base_library/features/http/service/Controller.h"
#include "base_library/features/http/service/ContentType.h"

class ExampleController : public Controller {
private:
    ADD_HANDLER_METHOD("/hello", Get, hello);

public:
    explicit ExampleController(std::shared_ptr<IAuthService> authService);
};

#endif  // CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H
