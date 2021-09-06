#ifndef CPP_BASE_LIBRARY_USERDTO_H
#define CPP_BASE_LIBRARY_USERDTO_H

#include <memory>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/controller/GroupDto.h"
#include "base_library/features/base/models/User.h"

class UserDto : public JsonSerializable {
 private:
  std::string m_firstName;
  std::string m_lastName;
  std::string m_eMail;
  std::string m_userName;
  std::string m_id;
  date::sys_time<std::chrono::microseconds> m_createdTimestamp;
  std::shared_ptr<GroupsDto> m_groups;

  static struct Shapes {
    const std::string FIRST_NAME = "first_name";
    const std::string LAST_NAME = "last_name";
    const std::string EMAIL = "email";
    const std::string USER_NAME = "user_name";
    const std::string CREATED_TIMESTAMP = "created_timestamp";
    const std::string GROUPS = "groups";
    const std::string ID = "id";
  } shape;

 public:
  [[nodiscard]] const std::string &getFirstName() const;
  [[nodiscard]] const std::string &getLastName() const;
  [[nodiscard]] const std::string &getEMail() const;
  [[nodiscard]] const std::string &getUserName() const;
  const std::string &getId() const;
  [[nodiscard]] const date::sys_time<std::chrono::microseconds>
      &getCreatedTimestamp() const;
  explicit UserDto(const User &user, std::string id = "");
  UserDto() = default;
  void serialize(
      rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;
  bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_USERDTO_H
