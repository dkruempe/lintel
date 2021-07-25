#include "base_library/features/http/ExampleController.h"
ExampleController::ExampleController() : Controller() {}

void ExampleController::helloGet(const httplib::Request& request,
                                 httplib::Response& response,
                                 const ContentType& contentType) {
  response.set_content("Hello World!", "text/plain");
}