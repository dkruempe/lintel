#ifndef LINTEL_USERMANAGEMENTCLICOMPONENT_H
#define LINTEL_USERMANAGEMENTCLICOMPONENT_H

#include <memory>
#include <optional>
#include <string>

#include "lintel/features/http/controllers/UserApi.h"
#include "lintel/features/cli/models/CommandLineComponent.h"
#include "lintel/features/cli/models/CommandParser.h"

/** CLI component for user and group management */
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
        UpdateUser,
        ChangePassword,
        ShowSessions,
        RevokeSession
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
    /*
     * Command: AllUsers
     * Command: AddUser
     */
    std::optional<std::string> m_userName;
    std::optional<std::string> m_firstName;
    std::optional<std::string> m_lastName;
    std::optional<std::string> m_eMail;
    std::optional<std::string> m_password;
    std::optional<std::string> m_oldPassword;
    std::optional<std::string> m_newPassword;
    std::optional<std::string> m_sessionId;
    std::optional<std::string> m_groupRemoved;

    CommandParser<Commands, Undefined> m_commandParser;

    /** Print formatted users to console */
    static void printUsers(const std::vector<UserDto> &users);

    /** Print formatted sessions to console */
    static void printSessions(const std::vector<UserSessionDto> &sessions);

    /** Print formatted groups to console */
    static void printGroups(const std::vector<GroupDto> &groups);

    /** Print user group memberships as a string */
    static std::string printUserGroups(const std::vector<GroupDto> &groups);

    /** Print subgroups of a group as a string */
    static std::string printSubGroups(const GroupDto &group);

public:
    /** @param userApi API for user and group operations */
    explicit UserManagementCliComponent(std::shared_ptr<UserApi> userApi);

    void onCommand(const UserDto &userDto, const std::string &input,
                   const std::vector<std::string> &parameters) override;

    void onHelp() override;

    void onShowMenu() override;

    bool onMenu(const std::string &component) override;

    bool onExit() override;

    void printCommandList(std::set<std::string> menuAlias) override;

    std::vector<std::string> allCommandsOf() override;
};

#endif  // LINTEL_USERMANAGEMENTCLICOMPONENT_H
