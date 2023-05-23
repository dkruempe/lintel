#include "base_library/features/base/repositories/UserRepository.h"

#include <date/tz.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"

UserRepository::UserRepository(
        std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations,
        std::shared_ptr<GroupRepository> groupRepository)
        : m_connectionConfigurations(std::move(connectionConfigurations)),
          m_connectionEntry(m_connectionConfigurations->ofDefault()),
          m_groupRepository(std::move(groupRepository)) {}

std::optional<User> UserRepository::of(const std::string &userName) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(userName);
    db::Result result = statement.execute(R"(
  select group_name,
         first_name,
         last_name,
         e_mail,
         password,
         created_timestamp
  from users u
  left join user_groups_relation ru
    on u.user_name = ru.user_name
  where u.user_name = ?)",
                                          builder);
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
    for (auto &iter: result) {
        std::string groupName = iter.of(0).getValue();
        if (groupName.empty()) {
            continue;
        }
        std::optional<Group> groupOpt = m_groupRepository->of(groupName);
        if (!groupOpt.has_value()) {
            LOG_ERROR("{} not available", iter.of(0).getValue());
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
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(user.getUserName());
    statement.execute(R"(
    delete from user_groups_relation
    where user_name = ?
  )",
                      builder);
    statement.execute(R"(
    delete from users
    where user_name = ?
  )",
                      builder);
}

void UserRepository::createOf(const User &user) {
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement statement(connection);
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(user.getUserName())
                .add(user.getPassword())
                .add(user.getEmail())
                .add(user.getFirstName())
                .add(user.getLastName())
                .add(user.getCreatedTimestamp());
        statement.execute(
                R"(
    insert into users (
    user_name,
    password,
    e_mail,
    first_name,
    last_name,
    created_timestamp)
    values (?, ?, ?, ?, ?, ?)
  )",
                builder);
    } catch (const db::SQLException &exception) {
        LOG_ERROR("cannot create new User {}", exception.what());
    } catch (const std::exception &exception) {
        LOG_ERROR("cannot create new User {}", exception.what());
    }
}

void UserRepository::addGroupsOf(const User &user,
                                 const std::set<Group> &groups) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::PreparedStatement statement(connection, R"(
    insert into user_groups_relation(user_name, group_name) values(?, ?))",
                                    "insert_group_relations");
    for (const auto &iter: groups) {
        if (user.has(iter)) {
            continue;
        }
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(user.getUserName()).add(iter.getGroupName());
        statement.execute(builder);
    }
}

void UserRepository::addGroupOf(const User &user, const Group &group) {
    auto found = std::find_if(user.getGroups().begin(), user.getGroups().end(),
                              [&](const Group &b) -> bool {
                                  return group.getGroupName() == b.getGroupName();
                              });
    if (found != user.getGroups().end()) {
        return;
    }
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(user.getUserName()).add(group.getGroupName());
    statement.execute(R"(
    insert into user_groups_relation(user_name, group_name) values(?, ?)
  )",
                      builder);
}

void UserRepository::removeGroupsOf(const User &user,
                                    const std::set<Group> &groups) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::PreparedStatement preparedStatement(connection, R"(
    delete from user_groups_relation where user_name = ? and group_name = ?)",
                                            "delete_user_group_relations");
    for (auto &iter: groups) {
        if (!user.has(iter)) {
            continue;
        }
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(user.getUserName()).add(iter.getGroupName());
        preparedStatement.execute(builder);
    }
}

void UserRepository::removeGroupOf(const User &user, const Group &group) {
    if (!user.has(group)) {
        return;
    }
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(user.getUserName()).add(group.getGroupName());
    statement.execute(R"(
    delete from user_groups_relation where user_name = ? and group_name = ?
  )",
                      builder);
}

std::vector<User> UserRepository::allOf(const std::string &userNameMatches) {
    std::vector<User> users;
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement statement(connection);
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(userNameMatches);
        db::Result result = statement.execute(R"(
    select ru.group_name,
           u.first_name,
           u.last_name,
           u.e_mail,
           u.password,
           u.created_timestamp,
           u.user_name
    from users u
    left join user_groups_relation ru
      on u.user_name = ru.user_name
    where u.user_name ~ ?
    order by u.user_name asc)",
                                              builder);
        std::string lastUserName;
        std::string lastFirstName;
        std::string lastLastName;
        std::string lastPassword;
        std::string lastEMail;
        date::sys_time<std::chrono::microseconds> lastCreatedTimestamp;
        std::vector<std::string> lastGroupNames;
        std::function<void()> func = [&]() {
            std::vector<Group> groups;
            for (const auto &groupName: lastGroupNames) {
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
        for (auto &iter: result) {
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
                lastGroupNames.clear();
            }
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
        func();
    } catch (const db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    } catch (const std::exception &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return users;
}

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
    from users u
    left join user_groups_relation ru
      on u.user_name = ru.user_name
    order by u.user_name asc)");
        std::string lastUserName;
        std::string lastFirstName;
        std::string lastLastName;
        std::string lastPassword;
        std::string lastEMail;
        date::sys_time<std::chrono::microseconds> lastCreatedTimestamp;
        std::vector<std::string> lastGroupNames;
        std::function<void()> func = [&]() {
            std::vector<Group> groups;
            for (const auto &groupName: lastGroupNames) {
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
        for (auto &iter: result) {
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
                lastGroupNames.clear();
            }
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
        func();
    } catch (const db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    } catch (const std::exception &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return users;
}

void UserRepository::changeFirstNameOf(const User &user,
                                       const std::string &firstName) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(firstName).add(user.getUserName());
    statement.execute(R"(
    update users
    set first_name = ?
    where user_name = ?
  )",
                      builder);
}

void UserRepository::changeLastNameOf(const User &user,
                                      const std::string &lastName) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(lastName).add(user.getUserName());
    statement.execute(R"(
    update users
    set last_name = ?
    where user_name = ?
  )",
                      builder);
}

void UserRepository::changeUserNameOf(const User &user,
                                      const std::string &userName) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(userName).add(user.getUserName());
    statement.execute(R"(
    update users
    set user_name = ?
    where user_name = ?
  )",
                      builder);
}

void UserRepository::changePasswordOf(const User &user,
                                      const std::string &password) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(password).add(user.getUserName());
    statement.execute(R"(
    update users
    set password = ?
    where user_name = ?
  )",
                      builder);
}

void UserRepository::changeEMailOf(const User &user, const std::string &eMail) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(eMail).add(user.getUserName());
    statement.execute(R"(
    update users
    set e_mail = ?
    where user_name = ?
  )",
                      builder);
}

void UserRepository::deleteOf(const std::vector<std::string> &userNames) {
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    db::Statement statement(connection);
    for (auto &userName: userNames) {
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(userName);
        statement.execute(R"(
    delete from user_groups_relation
    where user_name = ?
  )",
                          builder);
        statement.execute(R"(
    delete from users
    where user_name = ?
  )",
                          builder);
    }
}
