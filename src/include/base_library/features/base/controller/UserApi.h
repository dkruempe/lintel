#ifndef CPP_BASE_LIBRARY_USERAPI_H
#define CPP_BASE_LIBRARY_USERAPI_H

#include <memory>
#include <optional>
#include <vector>

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/base/controller/UserTokenDto.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/http/provider/ClientProvider.h"

class UserApi {
 private:
  std::shared_ptr<Client> m_client;

 public:
  explicit UserApi(const std::shared_ptr<ClientProvider> &clientProvider);

  // basic user functions for login / logout
  std::optional<UserDto> loginOf(const UserLoginDto &userLoginDto);
  bool logoutOf(const UserTokenDto &userLoginTokenDto);

  // user management functions
  std::vector<GroupDto> allOf();
  std::vector<GroupDto> allOf(const std::string &groupName);
  std::vector<GroupDto> allOf(const std::string &groupName,
                              bool isVirtualGroup);
  std::vector<GroupDto> allOf(bool isVirtualGroup);
  std::vector<UserDto> allUsersOf();
  void createOf(const UserDto &userDto);
};

#endif  // CPP_BASE_LIBRARY_USERAPI_H
