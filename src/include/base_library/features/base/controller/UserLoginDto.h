#ifndef CPP_BASE_LIBRARY_USERLOGINDTO_H
#define CPP_BASE_LIBRARY_USERLOGINDTO_H

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/services/AuthService.h"

class UserLoginDto : public JsonSerializable {
 private:
  std::string m_userName;
  std::string m_password;

  static struct Shapes {
    const std::string USER_NAME = "user_name";
    const std::string PASSWORD = "password";
  } shape;

 public:
  explicit UserLoginDto(const UserLogin &userLogin);
  UserLoginDto(std::string userName, std::string password);
  UserLoginDto() = default;
  [[nodiscard]] const std::string &getUserName() const;
  [[nodiscard]] const std::string &getPassword() const;
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERLOGINDTO_H
