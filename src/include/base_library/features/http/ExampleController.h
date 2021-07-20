#ifndef CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H
#define CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H

#include "base_library/features/http/service/Controller.h"

class ExampleController : public Controller {
 private:
  ADD_HANDLER_METHOD("/hello", Get, hello);
 public:
  ExampleController();
};

#endif  // CPP_BASE_LIBRARY_EXAMPLECONTROLLER_H
