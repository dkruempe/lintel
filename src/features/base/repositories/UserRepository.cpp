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
  std::string createdTimestamp = userArg.of(5).getValue();

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

  std::stringstream ss(createdTimestamp);
  date::sys_time<std::chrono::microseconds> lt;
  ss >> date::parse("%Y-%m-%d %H:%M:%S%Ez", lt);
  User user(firstName, lastName, User::Sex::Male, email, userName, password, lt,
            groups);
  return std::make_optional(user);
}

void UserRepository::updateOf(const User &user) {}

void UserRepository::deleteOf(const User &user) {}

void UserRepository::createOf(const User &user) {}