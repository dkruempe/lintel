#include "base_library/features/base/controller/UserApi.h"

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
UserApi::UserApi(const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
std::optional<UserDto> UserApi::loginOf(const UserLoginDto& userLoginDto) {
  const std::string body = userLoginDto.JsonSerializable::serialize();
  const httplib::Result& result =
      m_client->post("/user/login", body, "application/json");
  if (result->status != HttpStatusCodes::OK) {
    return std::nullopt;
  }
  UserDto userDto;
  userDto.JsonSerializable::deserialize(result.value().body);
  return std::make_optional<UserDto>(std::move(userDto));
}
bool UserApi::logoutOf(const UserTokenDto& userTokenDto) {
  const std::string body = userTokenDto.JsonSerializable::serialize();
  httplib::Result result =
      m_client->deletes("/user/logout", body, "application/json");
  return result->status == HttpStatusCodes::OK;
}
