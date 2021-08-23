#include <date/tz.h>

#include <chrono>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/features/base/repositories/UserRepository.h"

UserRepository::UserRepository(
    std::shared_ptr<ConnectionConfigurations> connectionConfigurations)
    : m_connectionConfigurations(std::move(connectionConfigurations)),
      m_connectionEntry(m_connectionConfigurations->of("DEFAULT")) {}

std::optional<User> UserRepository::of(const std::string &userName) {
  db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  db::Statement statement(connection);
  db::Result result = statement.execute(
      R"(select first_name,
                      last_name,
                      e_mail,
                      user_name,
                      password,
                      created_timestamp
               from public.user
               where user_name = ?
               fetch first row only)",
      {userName});
  if (result.getSize() > 1 || result.getSize() <= 0) {
    return std::nullopt;
  }
  db::Arguments arguments = result.of(0);
  std::string firstName = arguments.of("first_name").getValue();
  std::string lastName = arguments.of("last_name").getValue();
  std::string email = arguments.of("e_mail").getValue();
  std::string password = arguments.of("password").getValue();
  std::string createdTimestamp = arguments.of("created_timestamp").getValue();
  std::stringstream ss(createdTimestamp);
  date::sys_time<std::chrono::microseconds> lt;
  ss >> date::parse("%Y-%m-%d %H:%M:%S%Ez", lt);
  User user(firstName, lastName, User::Sex::Male, email, userName, password, lt,
            {});
  return std::make_optional(user);
}

void UserRepository::updateOf(const User &user) {}

void UserRepository::deleteOf(const User &user) {}

void UserRepository::createOf(const User &user) {}