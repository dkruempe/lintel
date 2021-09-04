#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/base/controller/UserDto.h"
UserApi::UserApi(const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
UserDto UserApi::loginOf(const UserLoginDto& userLoginDto) {
  const std::string body = userLoginDto.JsonSerializable::serialize();
  const httplib::Result& result =
      m_client->post("/user/login", body, "application/json");
  UserDto userDto;
  userDto.JsonSerializable::deserialize(result.value().body);
  return userDto;
}
