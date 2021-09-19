#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H


#include <memory>
#include <optional>

#include "base_library/features/base/controller/UserApi.h"

class AuthCliService {
 private:
  std::shared_ptr<UserApi> m_userApi;

  static std::string readPassword();

 public:
  explicit AuthCliService(std::shared_ptr<UserApi> userApi);
  UserDto onLogin();
  void onLogout(UserDto &&userDto);
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
