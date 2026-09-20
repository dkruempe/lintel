#include "base_library/features/base/repositories/UserRepository.h"

#include <date/date.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"

namespace {

/**
 * Aggregates joined user/group rows into User entities.
 * Rows must be ordered by user_name; consecutive rows of the same user are
 * merged, each user is resolved against the group repository.
 */
void appendUsersOf(db::Result &result,
                   const std::shared_ptr<GroupRepository> &groupRepository,
                   std::vector<User> &users) {
    std::string lastUserName;
    std::string lastFirstName;
    std::string lastLastName;
    std::string lastPassword;
    std::string lastEMail;
    date::sys_time<std::chrono::microseconds> lastCreatedTimestamp;
    std::vector<std::string> lastGroupNames;
    auto flush = [&]() {
        std::vector<Group> groups;
        for (const auto &groupName: lastGroupNames) {
            auto optGroup = groupRepository->of(groupName);
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
            flush();
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
    if (!lastUserName.empty()) {
        flush();
    }
}

}  // namespace

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
    transaction.commit();
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
        transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
}

std::vector<User> UserRepository::allOf(const std::string &userNameMatches) {
    std::vector<User> users;
    try {
        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement statement(connection);
        db::ParameterBuilder builder(m_connectionEntry);
        builder.add(userNameMatches);
        std::string statementString;
        switch (m_connectionEntry->getType()) {
            case db::ConnectionType::SQLite:
                statementString = R"(
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
            where u.user_name REGEXP ?
            order by u.user_name asc)";
                break;
            case db::ConnectionType::PostgreSQL:
                statementString = R"(
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
            where u.user_name ~* ?
            order by u.user_name asc)";
                break;
            default:
                throw db::SQLException("DatabaseType currently not supported");
        }
        db::Result result = statement.execute(statementString, builder);
        appendUsersOf(result, m_groupRepository, users);
    } catch (const db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    } catch (const std::exception &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return users;
}

Page<User> UserRepository::pageOf(const std::string &userNameMatches,
                                  const std::optional<std::string> &afterUserName,
                                  std::size_t limit) {
    std::vector<User> users;
    if (limit == 0) {
        return Page<User>(std::move(users), false, std::nullopt);
    }
    const bool hasFilter = !userNameMatches.empty();
    try {
        // Phase 1: resolve the page's user names via keyset pagination.
        std::string keyStatement =
                "select u.user_name from users u";
        db::ParameterBuilder keyBuilder(m_connectionEntry);
        if (hasFilter || afterUserName.has_value()) {
            keyStatement += " where ";
        }
        if (hasFilter) {
            keyStatement += m_connectionEntry->getType() ==
                                            db::ConnectionType::PostgreSQL
                                    ? "u.user_name ~* ?"
                                    : "u.user_name REGEXP ?";
            keyBuilder.add(userNameMatches);
        }
        if (afterUserName.has_value()) {
            keyStatement += hasFilter ? " and " : "";
            keyStatement += "u.user_name > ?";
            keyBuilder.add(afterUserName.value());
        }
        keyStatement += " order by u.user_name asc limit ?";
        keyBuilder.add(limit + 1);

        db::Connection connection(m_connectionEntry);
        db::Transaction transaction(connection);
        db::Statement statement(connection);
        db::Result keysResult = statement.execute(keyStatement, keyBuilder);
        std::vector<std::string> pageUserNames;
        const bool hasMore =
                static_cast<std::size_t>(keysResult.getSize()) > limit;
        const std::size_t pageSize =
                hasMore ? limit : static_cast<std::size_t>(keysResult.getSize());
        for (std::size_t i = 0; i < pageSize; i++) {
            pageUserNames.push_back(keysResult.of(i).of(0).getValue());
        }
        if (pageUserNames.empty()) {
            return Page<User>(std::move(users), false, std::nullopt);
        }
        const std::string &upperBound = pageUserNames.back();

        // Phase 2: load the full rows of exactly those user names so that a
        // user with multiple groups is never split across pages.
        std::string rowStatement = R"(
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
            where )";
        db::ParameterBuilder rowBuilder(m_connectionEntry);
        if (hasFilter) {
            rowStatement += m_connectionEntry->getType() ==
                                            db::ConnectionType::PostgreSQL
                                    ? "u.user_name ~* ? and "
                                    : "u.user_name REGEXP ? and ";
            rowBuilder.add(userNameMatches);
        }
        rowStatement += "u.user_name > ? and u.user_name <= ?";
        rowBuilder.add(afterUserName.value_or(std::string{}))
                .add(upperBound);
        rowStatement += " order by u.user_name asc";

        db::Result rowsResult = statement.execute(rowStatement, rowBuilder);
        appendUsersOf(rowsResult, m_groupRepository, users);
        transaction.commit();

        std::optional<std::string> nextAfter = std::nullopt;
        if (hasMore) {
            nextAfter = upperBound;
        }
        return Page<User>(std::move(users), hasMore, nextAfter);
    } catch (const db::SQLException &exception) {
        LOG_ERROR("{}", exception.what());
    } catch (const std::exception &exception) {
        LOG_ERROR("{}", exception.what());
    }
    return Page<User>(std::move(users), false, std::nullopt);
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
        appendUsersOf(result, m_groupRepository, users);
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
    transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
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
    transaction.commit();
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
        transaction.commit();
    }
}
