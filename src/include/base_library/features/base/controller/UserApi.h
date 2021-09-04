#ifndef CPP_BASE_LIBRARY_USERAPI_H
#define CPP_BASE_LIBRARY_USERAPI_H

#include <memory>
#include <optional>
#include <vector>

#include "base_library/features/base/controller/UserLoginDto.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/http/provider/ClientProvider.h"
#include "base_library/features/base/controller/UserDto.h"

class UserApi {
 private:
  std::shared_ptr<Client> m_client;

 public:
  explicit UserApi(const std::shared_ptr<ClientProvider> &clientProvider);

  UserDto loginOf(const UserLoginDto &userLoginDto);
};

#endif  // CPP_BASE_LIBRARY_USERAPI_H
