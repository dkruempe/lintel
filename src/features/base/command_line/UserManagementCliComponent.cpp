#include "base_library/features/base/command_line/UserManagementCliComponent.h"

#include <tabulate/table.hpp>
UserManagementCliComponent::UserManagementCliComponent(
    std::shared_ptr<UserApi> userApi)
    : CommandLineComponent(m_name, m_alias), m_userApi(std::move(userApi)) {
  m_commandParser.addCommand(
      Command("sag", "Show all available groups!")
          .addArgument(
              {"--group-name", "-g"}, &m_groupName,
              "Limits search result to given group name or pattern matching",
              true)
          .addArgument({"--is-virtual", "-v"}, &m_isVirtualGroup,
                       "Limits search result to virtual or non virtual groups"),
      AllGroups);
  m_commandParser.addCommand(
      Command("sau", "Show all available users!")
          .addArgument(
              {"--user-name", "-u"}, &m_userName,
              "Limits search result to given user name or pattern matching",
              true),
      AllUsers);
}
void UserManagementCliComponent::onCommand(
    const UserDto& userDto, const std::string& input,
    const std::vector<std::string>& parameters) {
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
      case AllGroups:
        if (!m_groupName.has_value() && !m_isVirtualGroup.has_value()) {
          printGroups(m_userApi->allOf());
        } else if (!m_groupName.has_value() && m_isVirtualGroup.has_value()) {
          printGroups(m_userApi->allOf(m_isVirtualGroup.value()));
        } else if (m_groupName.has_value() && !m_isVirtualGroup.has_value()) {
          printGroups(m_userApi->allOf(m_groupName.value()));
        } else {
          printGroups(
              m_userApi->allOf(m_groupName.value(), m_isVirtualGroup.value()));
        }
        break;
      case AllUsers: {
        printUsers(m_userApi->allUsersOf());
        break;
      }
      default:
        break;
    }
  } catch (const std::exception& exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}
void UserManagementCliComponent::onHelp() {
  m_commandParser.printHelp(getName(), getAlias(), m_description);
}
void UserManagementCliComponent::onShowMenu() {}
bool UserManagementCliComponent::onMenu(const std::string& component) {
  return true;
}
bool UserManagementCliComponent::onExit() { return true; }
void UserManagementCliComponent::printGroups(
    const std::vector<GroupDto>& groups) {
  tabulate::Table table;
  table.add_row({"No.", "Name", "IsVirtual", "SubGroups"});
  std::size_t iter = 0;
  for (const auto& group : groups) {
    table.add_row({std::to_string(++iter), group.getGroupName(),
                   group.isVirtual() ? "true" : "false",
                   printSubGroups(group)});
  }
  std::cout << table.str() << "\n";
}
std::string UserManagementCliComponent::printSubGroups(const GroupDto& group) {
  std::string printString;
  std::vector<GroupDto> subGroups = group.getSubGroups();
  for (std::size_t i = 0; i < subGroups.size(); i++) {
    GroupDto subGroup = subGroups[i];
    printString += subGroup.getGroupName();
    if (i < subGroups.size() - 1) {
      printString += ";";
    }
  }
  return printString;
}
void UserManagementCliComponent::printUsers(const std::vector<UserDto>& users) {
  tabulate::Table table;
  table.add_row(
      {"No.", "firstname", "lastname", "e-mail", "username", "groups"});
  std::size_t iter = 0;
  for (const auto& user : users) {
    table.add_row({std::to_string(++iter), user.getFirstName(),
                   user.getLastName(), user.getEMail(), user.getUserName(),
                   printUserGroups(user.getGroups())});
  }
  std::cout << table.str() << "\n";
}
std::string UserManagementCliComponent::printUserGroups(const std::vector<GroupDto>& groups) {
  std::string printString;
  for (std::size_t i = 0; i < groups.size(); i++) {
    GroupDto group = groups[i];
    printString += group.getGroupName();
    if (i < groups.size() - 1) {
      printString += ";";
    }
  }
  return printString;
}