#ifndef CPP_BASE_LIBRARY_USERCONTROLLER_H
#define CPP_BASE_LIBRARY_USERCONTROLLER_H

#include "base_library/features/http/service/Controller.h"
#include "base_library/features/base/services/AuthService.h"

class UserController : public Controller {
 private:
  std::shared_ptr<AuthService> m_authService;
  ADD_HANDLER_METHOD("/user/login", Post, loginOf);
  ADD_HANDLER_METHOD("/user/logout", Delete, logoutOf);
 public:
  explicit UserController(std::shared_ptr<AuthService> authService);
};

#endif  // CPP_BASE_LIBRARY_USERCONTROLLER_H
