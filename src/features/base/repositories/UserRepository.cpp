#include "base_library/features/base/repositories/UserRepository.h"

#include <date/tz.h>

#include <chrono>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"

UserRepository::UserRepository(
    std::shared_ptr<ConnectionConfigurations> connectionConfigurations,
    std::shared_ptr<GroupRepository> groupRepository)
    : m_connectionConfigurations(std::move(connectionConfigurations)),
      m_connectionEntry(m_connectionConfigurations->of("DEFAULT")),
      m_groupRepository(std::move(groupRepository)) {}

std::optional<User> UserRepository::of(const std::string &userName) {
  db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  db::Statement statement(connection);
  db::Result result = statement.execute(R"(
  select group_name,
         first_name,
         last_name,
         e_mail,
         password,
         created_timestamp
  from public.user u
  left join public.user_groups_relation ru
    on u.user_name = ru.user_name
  where u.user_name = ?)",
                                        {userName});
  if (result.getSize() <= 0) {
    return std::nullopt;
  }
  db::Arguments userArg = result.of(0);
  std::string firstName = userArg.of(1).getValue();
  std::string lastName = userArg.of(2).getValue();
  std::string email = userArg.of(3).getValue();
  std::string password = userArg.of(4).getValue();
  auto createdTimestamp =
      userArg.of(5).getValue<date::sys_time<std::chrono::microseconds>>();

  std::vector<Group> groups;
  for (auto &groupName : result) {
    std::optional<Group> groupOpt =
        m_groupRepository->of(groupName.of(0).getValue());
    if (!groupOpt.has_value()) {
      LOG_ERROR("{} not available", groupName.of(0).getValue());
      continue;
    }
    groups.push_back(groupOpt.value());
  }
  User user(firstName, lastName, User::Sex::Male, email, userName, password,
            groups, createdTimestamp);
  return std::make_optional(user);
}

void UserRepository::deleteOf(const User &user) {
  db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  db::Statement statement(connection);
  statement.execute(R"(
    delete from public.user_groups_relation
    where user_name = ?
  )",
                    {user.getUserName()});
  statement.execute(R"(
    delete from public.user
    where user_name = ?
  )",
                    {user.getUserName()});
}

void UserRepository::createOf(const User &user) {
  try {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    statement.execute(
        R"(
    insert into public.user (
    user_name,
    password,
    e_mail,
    first_name,
    last_name,
    created_timestamp)
    values (?, ?, ?, ?, ?, ?)
  )",
        {user.getUserName(), user.getPassword(), user.getEmail(),
         user.getFirstName(), user.getLastName(),
         date::format("%Y-%m-%d %H:%M:%S%Ez", user.getCreatedTimestamp())});
  } catch (const db::SQLException &exception) {
    LOG_ERROR("cannot create new User {}", exception.what());
  }
}

void UserRepository::changeUserOf(const User &user, const User &changeUser) {}
void UserRepository::addGroupOf(const User &user, const Group &group) {}
std::vector<User> UserRepository::allOf() {
  std::vector<User> users;
  try {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::Result result = statement.execute(R"(
    select ru.group_name,
           u.first_name,
           u.last_name,
           u.e_mail,
           u.password,
           u.created_timestamp,
           u.user_name
    from public.user u
    left join public.user_groups_relation ru
      on u.user_name = ru.user_name)");
    std::string lastUserName;
    std::string lastFirstName;
    std::string lastLastName;
    std::string lastPassword;
    std::string lastEMail;
    date::sys_time<std::chrono::microseconds> lastCreatedTimestamp;
    std::vector<std::string> lastGroupNames;
    std::function<void()> func = [&]() {
      std::vector<Group> groups;
      for (const auto &groupName : lastGroupNames) {
        auto optGroup = m_groupRepository->of(groupName);
        if (!optGroup.has_value()) {
          LOG_ERROR("{} not available", groupName);
          continue;
        }
        groups.push_back(optGroup.value());
      }
      User user(lastFirstName, lastLastName, User::Sex::Male, lastEMail,
                lastUserName, lastPassword, groups, lastCreatedTimestamp);
      users.push_back(user);
    };
    for (auto &iter : result) {
      std::string groupName = iter.of(0).getValue();
      std::string firstName = iter.of(1).getValue();
      std::string lastName = iter.of(2).getValue();
      std::string eMail = iter.of(3).getValue();
      std::string password = iter.of(4).getValue();
      auto createdTimestamp =
          iter.of(5).getValue<date::sys_time<std::chrono::microseconds>>();
      std::string userName = iter.of(6).getValue();
      if (lastUserName != userName && !lastUserName.empty()) {
        func();
      }
      if (lastUserName != userName) {
        lastUserName = userName;
        if (!groupName.empty()) {
          lastGroupNames.push_back(groupName);
        }
        lastFirstName = firstName;
        lastLastName = lastName;
        lastEMail = eMail;
        lastPassword = password;
        lastCreatedTimestamp = createdTimestamp;
      }
    }
    func();
  } catch (const db::SQLException &exception) {
    LOG_ERROR("{}", exception.what());
  }
  return users;
}
