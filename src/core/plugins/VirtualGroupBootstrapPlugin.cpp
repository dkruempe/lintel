#include "base_library/core/plugins/VirtualGroupBootstrapPlugin.h"

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Transaction.h"
#include "base_library/core/services/LoggerService.h"

VirtualGroupBootstrapPlugin::VirtualGroupBootstrapPlugin(
    const std::shared_ptr<ConnectionConfigurations> &connectionConfigurations,
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
  db::PreparedStatement statement(
      connection,
      "insert into public.group(name, virtual) values(?, ?) ON CONFLICT ON "
      "CONSTRAINT group_pk DO NOTHING",
      "insert_group");
  for (const auto &group : groups) {
    statement.execute(
        {group.getGroupName(), db::Serialization<bool>::serialize(
                                   group.isVirtual(), m_connectionEntry)});
  }
  statement.close();
  LOG_INFO("start");
}
