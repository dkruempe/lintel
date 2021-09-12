#ifndef CPP_BASE_LIBRARY_USERMANAGEMENTCLICOMPONENT_H
#define CPP_BASE_LIBRARY_USERMANAGEMENTCLICOMPONENT_H

#include <memory>
#include <optional>
#include <string>

#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/cli/models/CommandParser.h"

class UserManagementCliComponent : public CommandLineComponent {
 private:
  static constexpr std::string_view m_name = "UserManagement";
  static constexpr std::string_view m_alias = "UserM";
  static constexpr std::string_view m_description =
      "This component is responsible for the total user and group management";
  std::shared_ptr<UserApi> m_userApi;

  enum Commands {
    Undefined,
    // group operations
    AllGroups,
    AddGroup,
    RemoveGroup,
    UpdateGroup,
    // user operations
    AllUsers,
    AddUser,
    RemoveUser,
    UpdateUser
  };

  // Flags
  /*
   * Command: AllGroups
   * GroupName for limiting search result of AllGroups Command
   * - support regex expressions
   * - empty implies no flag set
   * IsVirtualGroup:
   * -
   */
  std::optional<std::string> m_groupName;
  std::optional<bool> m_isVirtualGroup;

  CommandParser<Commands, Undefined> m_commandParser;
  void printGroups(const std::vector<GroupDto> &groups);
  static std::string printSubGroups(const GroupDto &group);

 public:
  explicit UserManagementCliComponent(std::shared_ptr<UserApi> userApi);

  void onCommand(const UserDto &userDto, const std::string &input,
                 const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
};

#endif  // CPP_BASE_LIBRARY_USERMANAGEMENTCLICOMPONENT_H
