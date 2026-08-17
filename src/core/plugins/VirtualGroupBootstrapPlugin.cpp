#include "base_library/core/plugins/VirtualGroupBootstrapPlugin.h"

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"
#include "base_library/features/base/provider/GroupProvider.h"

VirtualGroupBootstrapPlugin::VirtualGroupBootstrapPlugin(
        const std::shared_ptr<DatabaseConnectionConfigurations>
        &connectionConfigurations,
        std::shared_ptr<Hypodermic::Container> container)
        : m_container(std::move(container)),
          m_connectionEntry(connectionConfigurations->ofDefault()) {}

BootstrapSequence VirtualGroupBootstrapPlugin::getPriority() {
    return BootstrapSequence::VirtualGroups;
}

void VirtualGroupBootstrapPlugin::onStart() {
    if (m_connectionEntry == nullptr) {
        LOG_INFO("no default database connection - skip virtual group bootstrap");
        return;
    }
    auto groupProviders = m_container->resolveAll<GroupProvider>();
    std::set<Group> groups;
    for (const auto &groupProvider: groupProviders) {
        std::vector<Group> temp = groupProvider->provide();
        groups.insert(temp.begin(), temp.end());
    }
    db::Connection connection(m_connectionEntry);
    db::Transaction transaction(connection);
    {
        std::string stmt;
        switch (m_connectionEntry->getType()) {
            case db::ConnectionType::PostgreSQL:
                stmt = R"(insert into groups(name, virtual)
                  values(?, ?)
                  ON CONFLICT
                  ON CONSTRAINT group_pk DO NOTHING)";
                break;
            case db::ConnectionType::SQLite:
                stmt = R"(insert or ignore into groups(name, virtual)
                  values(?, ?))";
                break;
            default:
                throw std::runtime_error("unknown db type");
        }
        db::PreparedStatement statement(connection, stmt, "insert_group");
        for (const auto &group: groups) {
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add(group.getGroupName()).add(group.isVirtual());
            statement.execute(builder);
        }
        statement.close();
    }
    /*
     * HACKY implementation of adding default base groups directly to exising
     * groups like User or Admin
     *
     * Future Solution:
     * Remove this HACK Workaround and add parameter to defining if the mentioned
     * group is an admin group or an user group.
     */

    {
        std::string stmt;
        switch (m_connectionEntry->getType()) {
            case db::ConnectionType::PostgreSQL:
                stmt = R"(
            insert into group_groups_relation
              (group_name,
               base_group_name)
            values(?, ?)
            ON CONFLICT
            ON CONSTRAINT group_groups_relation_pk
            DO NOTHING)";
                break;
            case db::ConnectionType::SQLite:
                stmt = R"(
            insert or ignore into group_groups_relation
              (group_name,
               base_group_name)
            values(?, ?))";
                break;
            default:
                throw std::runtime_error("unknown db type");
        }
        db::PreparedStatement statement(connection, stmt, "add_relation");
        for (const auto &group: groups) {
            bool isAdminGroup =
                    StringUtils::startsWith(group.getGroupName(), "Admin");
            if (isAdminGroup) {
                db::ParameterBuilder builder(m_connectionEntry);
                builder.add<std::string>("Admin").add(group.getGroupName());
                statement.execute(builder);
                continue;
            }
            bool isUserGroup = StringUtils::startsWith(group.getGroupName(), "User");
            if (isUserGroup) {
                db::ParameterBuilder builder(m_connectionEntry);
                builder.add<std::string>("User").add(group.getGroupName());
                statement.execute(builder);
                continue;
            }
        }
    }
    transaction.commit();
    LOG_INFO("start");
}
