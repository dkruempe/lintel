#include "base_library/core/plugins/AdminUserBootstrapPlugin.h"

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/Cryption.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace {
constexpr const char *kAdminUserNameEnv = "ADMIN_USERNAME";
constexpr const char *kAdminPasswordEnv = "ADMIN_PASSWORD";
constexpr const char *kAdminGroupName = "Admin";

std::optional<std::string> envOf(const char *name) {
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
}
}  // namespace

AdminUserBootstrapPlugin::AdminUserBootstrapPlugin(
        std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations)
        : m_connectionConfigurations(std::move(connectionConfigurations)) {}

BootstrapSequence AdminUserBootstrapPlugin::getPriority() {
    return BootstrapSequence::AdminUser;
}

void AdminUserBootstrapPlugin::onStart() {
    const std::optional<std::string> password = envOf(kAdminPasswordEnv);
    if (!password.has_value()) {
        LOG_INFO("{} not set - skip initial admin bootstrap", kAdminPasswordEnv);
        return;
    }
    const std::string userName = envOf(kAdminUserNameEnv).value_or("admin");
    const std::shared_ptr<DatabaseConnectionEntry> connectionEntry =
            m_connectionConfigurations->ofDefault();
    if (connectionEntry == nullptr) {
        LOG_ERROR("no default database connection - skip admin bootstrap");
        return;
    }
    try {
        db::Connection connection(connectionEntry);
        db::Statement statement(connection);
        db::ParameterBuilder builder(connectionEntry);
        builder.add(userName);
        db::Result result =
                statement.execute("select user_name from users where user_name = ?",
                                  builder);
        if (result.getSize() > 0) {
            LOG_INFO("user {} already exists - skip admin bootstrap", userName);
            return;
        }
        db::ParameterBuilder groupBuilder(connectionEntry);
        groupBuilder.add(std::string(kAdminGroupName));
        db::Result groupResult = statement.execute(
                "select group_name from groups where group_name = ?",
                groupBuilder);
        if (groupResult.getSize() <= 0) {
            LOG_ERROR("group {} does not exist - skip admin bootstrap",
                      kAdminGroupName);
            return;
        }
        db::ParameterBuilder userBuilder(connectionEntry);
        userBuilder.add(userName)
                .add(Cryption::hashOf(password.value()))
                .add(std::string())
                .add(std::string("Admin"))
                .add(std::string("User"))
                .add(std::chrono::time_point_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now()));
        db::Transaction transaction(connection);
        statement.execute(R"(
      insert into users (user_name, password, e_mail, first_name, last_name,
                         created_timestamp)
      values (?, ?, ?, ?, ?, ?)
    )",
                          userBuilder);
        db::ParameterBuilder relationBuilder(connectionEntry);
        relationBuilder.add(userName).add(std::string(kAdminGroupName));
        statement.execute(R"(
      insert into user_groups_relation (user_name, group_name)
      values (?, ?)
    )",
                          relationBuilder);
        LOG_INFO("initial admin user {} created", userName);
    } catch (std::exception &exception) {
        LOG_ERROR("failed to create initial admin user - {}", exception.what());
    }
}
