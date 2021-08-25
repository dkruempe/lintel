#include "base_library/features/base/models/User.h"

#include <utility>

const std::string& User::getFirstName() const { return m_firstName; }
const std::string& User::getLastName() const { return m_lastName; }
User::Sex User::getSex() const { return m_sex; }
const std::string& User::getEmail() const { return m_email; }
const std::string& User::getUserName() const { return m_userName; }
const std::string& User::getPassword() const { return m_password; }
const std::vector<Group>& User::getGroups() const { return m_groups; }
const date::sys_time<std::chrono::microseconds>& User::getCreatedTimestamp()
    const {
  return m_createdTimestamp;
}
User::User(std::string firstName, std::string lastName, Sex sex,
           std::string email, std::string userName, std::string password,
           std::vector<Group> groups,
           date::sys_time<std::chrono::microseconds> createdTimestamp)
    : m_firstName(std::move(firstName)),
      m_lastName(std::move(lastName)),
      m_sex(sex),
      m_email(std::move(email)),
      m_userName(std::move(userName)),
      m_password(std::move(password)),
      m_createdTimestamp(createdTimestamp),
      m_groups(std::move(groups)) {}
std::ostream& operator<<(std::ostream& os, const User& user) {
  os << "User{"
     << "username: " << user.m_userName << ", first_name: " << user.m_firstName
     << ", last_name: " << user.m_lastName << ", e_mail: " << user.m_email
     << ", password: " << user.m_password << ", sex: " << user.m_sex
     << ", created_timestamp: "
     << date::format("%Y.%m.%d %T%Ez", user.m_createdTimestamp)
     << ", groups = {";
  for (std::size_t i = 0; i < user.m_groups.size(); i++) {
    const Group& group = user.m_groups[i];
    os << group;
    if (i != user.m_groups.size() - 1) {
      os << ", ";
    }
  }
  os << "}";
  return os;
}
