#include "base_library/features/base/controller/UserApi.h"

#include "base_library/features/base/controller/UserDto.h"
#include "base_library/features/http/service/HttpStatusCodes.h"
UserApi::UserApi(const std::shared_ptr<ClientProvider>& clientProvider)
    : m_client(clientProvider->provide()) {}
std::optional<UserDto> UserApi::loginOf(const UserLoginDto& userLoginDto) {
  m_client->setBasicAuth(userLoginDto.getUserName(),
                         userLoginDto.getPassword());
  const httplib::Result& result =
      m_client->post("/user/login", "", "application/json");
  if (result->status != HttpStatusCodes::OK) {
    return std::nullopt;
  }
  UserDto userDto;
  userDto.JsonSerializable::deserialize(result.value().body);
  m_client->setBasicAuth("", "");
  m_client->setBearerTokenAuth(userDto.getId());
  return std::make_optional<UserDto>(std::move(userDto));
}
bool UserApi::logoutOf(const UserTokenDto& userTokenDto) {
  httplib::Result result =
      m_client->deletes("/user/logout", "", "application/json");
  return result->status == HttpStatusCodes::OK;
}
