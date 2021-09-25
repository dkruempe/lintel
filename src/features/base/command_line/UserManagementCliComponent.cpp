#include "base_library/features/base/command_line/UserManagementCliComponent.h"

#include <tabulate/table.hpp>

#include "base_library/features/base/configuration/Cryption.h"
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
  m_commandParser.addCommand(
      Command("user_add", "Creates user!")
          .addArgument({"--first-name", "-f"}, &m_firstName,
                       "Firstname of user", false)
          .addArgument({"--last-name", "-l"}, &m_lastName, "lastname of user",
                       false)
          .addArgument({"--email", "-e"}, &m_eMail, "email of user", false)
          .addArgument({"--password", "-p"}, &m_password, "password of user",
                       false)
          .addArgument({"--username", "-u"}, &m_userName, "username of user",
                       false),
      AddUser);
  m_commandParser.addCommand(
      Command("user_update", "Update user!")
          .addArgument({"--group-add", "-g"}, &m_groupName,
                       "Group which should be added to user!", true)
          .addArgument({"--group-remove", "-gr"}, &m_groupRemoved,
                       "Group which should be removed from user!", true)
          .addArgument({"--user", "-u"}, &m_userName,
                       "User Name which should be updated", false),
      UpdateUser);
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
      case AddUser: {
        if (!m_firstName.has_value() || !m_lastName.has_value() ||
            !m_eMail.has_value() || !m_userName.has_value() ||
            !m_password.has_value()) {
          std::cerr << "ERROR: please make sure that all arguments are "
                       "correctly filled\n";
        }
        User user(m_firstName.value(), m_lastName.value(), User::Sex::Male,
                  m_eMail.value(), m_userName.value(), m_password.value(), {});
        UserDto userDtoNew(user);
        userDtoNew.setPassword(Cryption::encodeBase64(m_password.value()));
        m_userApi->createOf(userDtoNew);
        break;
      }
      case UpdateUser: {
        std::set<std::string> groupAdds;
        std::set<std::string> groupRemoves;
        if (m_groupName.has_value()) {
          groupAdds.insert(m_groupName.value());
        }
        if (m_groupRemoved.has_value()) {
          groupRemoves.insert(m_groupRemoved.value());
        }
        m_userApi->updateOf(m_userName.value(), groupAdds, groupRemoves);
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
std::string UserManagementCliComponent::printUserGroups(
    const std::vector<GroupDto>& groups) {
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
void UserManagementCliComponent::printCommandList() {
  m_commandParser.printCommandList();
}
