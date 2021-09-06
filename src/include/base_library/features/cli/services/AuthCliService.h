#ifndef CPP_BASE_LIBRARY_AUTHCLISERVICE_H
#define CPP_BASE_LIBRARY_AUTHCLISERVICE_H

#include <termios.h>

#include <memory>
#include <optional>

#include "base_library/features/base/controller/UserApi.h"

class AuthCliService {
 private:
  std::shared_ptr<UserApi> m_userApi;
  std::string m_clear = std::string(100, '\n');

  static std::string readPassword();
  static void hideStdinKeystrokes();
  static void showStdinKeystrokes();

 public:
  explicit AuthCliService(std::shared_ptr<UserApi> userApi);
  UserDto onLogin();
  void onLogout(UserDto &&userDto);
};

#endif  // CPP_BASE_LIBRARY_AUTHCLISERVICE_H
