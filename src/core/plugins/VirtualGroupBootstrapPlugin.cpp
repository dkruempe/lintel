#include "base_library/core/plugins/VirtualGroupBootstrapPlugin.h"

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/StringUtils.h"

VirtualGroupBootstrapPlugin::VirtualGroupBootstrapPlugin(
    const std::shared_ptr<DatabaseConnectionConfigurations>
        &connectionConfigurations,
    std::vector<std::shared_ptr<GroupProvider>> groupProviders)
    : m_groupProviders(std::move(groupProviders)),
      m_connectionEntry(connectionConfigurations->of("DEFAULT")) {}
BootstrapSequence VirtualGroupBootstrapPlugin::getPriority() {
  return BootstrapSequence::VirtualGroups;
}
void VirtualGroupBootstrapPlugin::onStart() {
  std::set<Group> groups;
  for (const auto &groupProvider : m_groupProviders) {
    std::vector<Group> temp = groupProvider->provide();
    groups.insert(temp.begin(), temp.end());
  }
  db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  {
    db::PreparedStatement statement(
        connection,
        "insert into public.group(name, virtual) values(?, ?) ON CONFLICT ON "
        "CONSTRAINT group_pk DO NOTHING",
        "insert_group");
    for (const auto &group : groups) {
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
    db::PreparedStatement statement(
        connection,
        "insert into public.group_groups_relation(group_name, base_group_name) "
        "values(?, ?) ON CONFLICT ON CONSTRAINT group_groups_relation_pk DO "
        "NOTHING",
        "add_relation");
    for (const auto &group : groups) {
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
  LOG_INFO("start");
}
