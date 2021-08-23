#include "base_library/features/base/repositories/GroupRepository.h"

#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/services/LoggerService.h"

GroupRepository::GroupRepository(
    std::shared_ptr<ConnectionConfigurations> connectionConfigurations)
    : m_connectionConfigurations(std::move(connectionConfigurations)),
      m_connectionEntry(m_connectionConfigurations->of("DEFAULT")) {
  initGroups();
}
void GroupRepository::initGroups() {
  db::Connection connection(m_connectionEntry);
  db::Statement statement(connection);
  db::Result result =
      statement.execute("select name, virtual from public.group");
  for (const auto &iter : result) {
    std::string groupName = iter.of("name").getValue();
    bool isVirtual = iter.of("virtual").getValue<bool>();
    Group group(groupName, {}, isVirtual);
    std::cout << group << std::endl;
  }
}
