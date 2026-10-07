#ifndef LINTEL_EXAMPLECONTROLLER_H
#define LINTEL_EXAMPLECONTROLLER_H

#include "lintel/features/http/service/Controller.h"
#include "lintel/features/http/service/ContentType.h"

/** Example controller demonstrating the HTTP handler registration pattern */
class ExampleController : public Controller {
private:
    ADD_HANDLER_METHOD("/hello", Get, hello);

public:
    explicit ExampleController(std::shared_ptr<IAuthService> authService);
};

#endif  // LINTEL_EXAMPLECONTROLLER_H
