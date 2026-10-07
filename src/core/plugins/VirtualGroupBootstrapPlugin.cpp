#include "lintel/core/plugins/VirtualGroupBootstrapPlugin.h"

#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/PreparedStatement.h"
#include "lintel/core/persistence/Transaction.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/StringUtils.h"
#include "lintel/features/base/provider/GroupProvider.h"

VirtualGroupBootstrapPlugin::VirtualGroupBootstrapPlugin(
  const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations,
  std::shared_ptr<Hypodermic::Container> container)
  : m_container(std::move(container)), m_connectionEntry(connectionConfigurations->ofDefault())
{}

BootstrapSequence VirtualGroupBootstrapPlugin::getPriority() { return BootstrapSequence::VirtualGroups; }

void VirtualGroupBootstrapPlugin::onStart()
{
  if (m_connectionEntry == nullptr) {
    LOG_INFO("no default database connection - skip virtual group bootstrap");
    return;
  }
  auto groupProviders = m_container->resolveAll<GroupProvider>();
  std::set<Group> groups;
  for (const auto &groupProvider : groupProviders) {
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
    for (const auto &group : groups) {
      db::ParameterBuilder builder(m_connectionEntry);
      builder.add(group.getGroupName()).add(group.isVirtual());
      statement.execute(builder);
    }
    statement.close();
  }
  /*
   * Deliberate workaround, not an oversight: attaches the default base groups
   * (User, Admin) to the groups that exist for a user, by classifying them
   * through their name prefix rather than through a configuration flag.
   *
   * Why it is like this: `group_groups_relation` is the membership table, and
   * the base groups only exist once a user has been created, so the relation
   * rows cannot be written by the plugin that seeds the groups - it runs
   * earlier (BootstrapSequence::VirtualGroups, after Database and
   * MessageQueue). This block therefore back-fills the membership when the
   * first user appears.
   *
   * Known limitation: a user-defined group whose name starts with "Admin" or
   * "User" is classified by that prefix, which is wrong for such a group.
   *
   * Planned fix: carry the kind (admin group vs. user group) as data instead
   * of deriving it from the name, so the membership is written by the group
   * seed rather than reconstructed from a prefix here. That needs a schema
   * column and a configuration attribute, i.e. a migration, which is why it
   * is not done in this pass.
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
    for (const auto &group : groups) {
      bool isAdminGroup = StringUtils::startsWith(group.getGroupName(), "Admin");
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
