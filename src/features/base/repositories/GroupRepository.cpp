#include "base_library/features/base/repositories/GroupRepository.h"

#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
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
  for (const auto& iter : result) {
    std::string groupName = iter.of("name").getValue();
    bool isVirtual = iter.of("virtual").getValue<bool>();
    Group group(groupName, {}, isVirtual);
    m_groups.push_back(group);
    m_groupMap.insert({group.getGroupName(), group});
  }
}
std::optional<Group> GroupRepository::of(const std::string& groupName) {
  auto found = m_groupMap.find(groupName);
  if (found == m_groupMap.end()) {
    return std::nullopt;
  }
  return std::make_optional(found->second);
}
std::vector<Group> GroupRepository::allOf() { return m_groups; }
void GroupRepository::createOf(const Group& group) {
  if (group.isVirtual()) {
    throw std::runtime_error(
        "group cannot be virtual. Those groups has to be defined in the code "
        "itself");
  }
  auto found = m_groupMap.find(group.getGroupName());
  if (found != m_groupMap.end()) {
    throw std::runtime_error(
        "There is an existing group with the mentioned name");
  }
  db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(
      connection, "insert into public.group(name, virtual) values(?, ?)",
      "insert_group");
  preparedStatement.execute(
      {group.getGroupName(), group.isVirtual() ? "t" : "f"});
  m_groupMap.insert({group.getGroupName(), group});
  m_groups.push_back(group);
}
void GroupRepository::deleteOf(const Group& group) {
  db::Connection connection(m_connectionEntry);
  db::Statement statement(connection);
  db::Result result = statement.execute(
      "select user_name, group_name from public.user_groups_relation where "
      "group_name = ?",
      {group.getGroupName()});
  if (result.getSize() > 0) {
    throw std::runtime_error(
        "cannot remove group which is in usage of an user");
  }
  statement.execute("delete from public.group where name = ?",
                    {group.getGroupName()});
}
