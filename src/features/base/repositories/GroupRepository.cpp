#include "base_library/features/base/repositories/GroupRepository.h"

#include <regex>
#include <utility>

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/Statement.h"
#include "base_library/core/services/LoggerService.h"

GroupRepository::GroupRepository(
        std::shared_ptr<DatabaseConnectionConfigurations> connectionConfigurations)
        : m_connectionConfigurations(std::move(connectionConfigurations)),
          m_connectionEntry(m_connectionConfigurations->ofDefault()) {}

void GroupRepository::initGroups() {
    db::Connection connection(m_connectionEntry);
    db::Statement statement(connection);
    // initialize virtual groups
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(true);
    db::Result result = statement.execute(R"(
  select name,
         virtual
  from groups
  where virtual = ?
  )",
                                          builder);
    for (auto &iter: result) {
        std::string groupName = iter.of(0).getValue();
        bool isVirtual = iter.of(1).getValue<bool>();
        if (groupName.empty()) {
            continue;
        }
        Group group(groupName, {}, isVirtual);
        m_groupMap.insert({group.getGroupName(), group});
        m_groups.push_back(group);
    }
    db::ParameterBuilder builder1(m_connectionEntry);
    builder1.add(false);
    // initialize non virtual groups
    result = statement.execute(R"(
  select g.name,
         g.virtual,
         gr.base_group_name
  from groups g
  left join group_groups_relation gr
    on gr.group_name = g.name
  where g.virtual = ?
  order by g.name;
  )",
                               builder1);
    std::string lastGroupName;
    bool lastIsVirtual;
    std::vector<std::string> groupNames;
    for (const auto &iter: result) {
        std::string groupName = iter.of(0).getValue();
        bool isVirtual = iter.of(1).getValue<bool>();
        std::string groupMember = iter.of(2).getValue();
        if (groupName.empty()) {
            continue;
        }
        if (lastGroupName != groupName && !lastGroupName.empty()) {
            std::vector<Group> groups;
            std::transform(groupNames.begin(), groupNames.end(),
                           std::back_inserter(groups),
                           [&](const std::string &groupName) -> Group {
                               return m_groupMap.at(groupName);
                           });
            Group group(lastGroupName, groups, lastIsVirtual);
            m_groupMap.insert({group.getGroupName(), group});
            m_groups.push_back(group);
        }
        if (lastGroupName != groupName) {
            lastGroupName = groupName;
            lastIsVirtual = isVirtual;
            groupNames.clear();
        }
        if (!groupMember.empty()) {
            groupNames.push_back(groupMember);
        }
    }
    std::vector<Group> groups;
    if (lastGroupName.empty()) {
        return;
    }
    std::transform(groupNames.begin(), groupNames.end(),
                   std::back_inserter(groups),
                   [&](const std::string &groupName) -> Group {
                       return m_groupMap.at(groupName);
                   });
    Group group(lastGroupName, groups, lastIsVirtual);
    m_groupMap.insert({group.getGroupName(), group});
    m_groups.push_back(group);
}

std::optional<Group> GroupRepository::of(const std::string &groupName) {
    auto found = m_groupMap.find(groupName);
    if (found == m_groupMap.end()) {
        return std::nullopt;
    }
    return std::make_optional(found->second);
}

std::vector<Group> GroupRepository::allOf() { return m_groups; }

std::vector<Group> GroupRepository::allOf(bool isVirtualGroup) {
    std::vector<Group> temp(m_groups);
    temp.erase(std::remove_if(temp.begin(), temp.end(),
                              [&isVirtualGroup](const Group &group) -> bool {
                                  return group.isVirtual() != isVirtualGroup;
                              }),
               temp.end());
    return temp;
}

std::vector<Group> GroupRepository::allOf(const std::string &groupName) {
    std::regex match(groupName);
    std::vector<Group> temp(m_groups);
    temp.erase(std::remove_if(temp.begin(), temp.end(),
                              [&match](const Group &group) -> bool {
                                  return !std::regex_match(group.getGroupName(),
                                                           match);
                              }),
               temp.end());
    return temp;
}

std::vector<Group> GroupRepository::allOf(const std::string &groupName,
                                          bool isVirtualGroup) {
    std::vector<Group> temp = allOf(isVirtualGroup);
    if (temp.empty()) {
        return temp;
    }
    std::regex match(groupName);
    temp.erase(std::remove_if(temp.begin(), temp.end(),
                              [&match](const Group &group) -> bool {
                                  return !std::regex_match(group.getGroupName(),
                                                           match);
                              }),
               temp.end());
    return temp;
}

void GroupRepository::createOf(const Group &group) {
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
            connection, "insert into groups(name, virtual) values(?, ?)",
            "insert_group");
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(group.getGroupName()).add(group.isVirtual());
    preparedStatement.execute(builder);
    m_groupMap.insert({group.getGroupName(), group});
    m_groups.push_back(group);
}

void GroupRepository::deleteOf(const Group &group) {
    db::Connection connection(m_connectionEntry);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(group.getGroupName());
    db::Result result = statement.execute(
            "select user_name, group_name from public.user_groups_relation where "
            "group_name = ?",
            builder);
    if (result.getSize() > 0) {
        throw std::runtime_error(
                "cannot remove group which is in usage of an user");
    }
    statement.execute("delete from groups where name = ?", builder);
}

void GroupRepository::addGroupOf(const Group &group, const Group &add) {
    auto found = std::find_if(group.getGroups().begin(), group.getGroups().end(),
                              [&](const Group &b) -> bool {
                                  return b.getGroupName() == add.getGroupName();
                              });
    if (found != group.getGroups().end()) {
        return;
    }
    if (group.isVirtual()) {
        LOG_ERROR("{} is a virtual group", group.getGroupName());
        return;
    }
    if (!add.isVirtual()) {
        LOG_ERROR("{} is not a virtual group", add.getGroupName());
        return;
    }
    db::Connection connection(m_connectionEntry);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(group.getGroupName()).add(add.getGroupName());
    db::Result result = statement.execute(R"(
    insert into group_groups_relation(group_name, base_group_name) values(?,?)
  )",
                                          builder);
}

void GroupRepository::removeGroupOf(const Group &group, const Group &remove) {
    auto found = std::find_if(group.getGroups().begin(), group.getGroups().end(),
                              [&](const Group &b) -> bool {
                                  return b.getGroupName() == remove.getGroupName();
                              });
    if (found == group.getGroups().end()) {
        return;
    }
    db::Connection connection(m_connectionEntry);
    db::Statement statement(connection);
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(group.getGroupName()).add(remove.getGroupName());
    db::Result result = statement.execute(
            R"(
    delete from group_groups_relation(group_name, base_group_name) where group_name = ? and base_group_name = ?
      )",
            builder);
}

void GroupRepository::onAwake() { initGroups(); }
