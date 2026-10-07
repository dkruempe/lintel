#include "lintel/features/cli/components/UserManagementCliComponent.h"

#include <lintel/core/utils/TableBuilder.h>

#include "lintel/core/services/StringifyService.h"
#include "lintel/core/utils/Cryption.h"

UserManagementCliComponent::UserManagementCliComponent(
        std::shared_ptr<UserApi> userApi)
        : CommandLineComponent(m_name, m_alias), m_userApi(std::move(userApi)) {
    m_commandParser.addCommand(
            Command("show_groups", "Show all available groups!")
                    .addArgument(
                            {"--group-name", "-g"}, &m_groupName,
                            "Limits search result to given group name or pattern matching",
                            true)
                    .addArgument({"--is-virtual", "-v"}, &m_isVirtualGroup,
                                 "Limits search result to virtual or non virtual groups"),
            AllGroups);
    m_commandParser.addCommand(
            Command("show_users", "Show all available users!")
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
    m_commandParser.addCommand(
            Command("user_delete", "Delete user!")
                    .addArgument({"--user-name", "-u"}, &m_userName,
                                 "Username which should be deleted!", false),
            RemoveUser);
    m_commandParser.addCommand(
            Command("user_change_password", "Changes the password of a user!")
                    .addArgument({"--username", "-u"}, &m_userName,
                                 "Username of the user", false)
                    .addArgument({"--old-password", "-o"}, &m_oldPassword,
                                 "Current password of the user (required for "
                                 "self-service, not for admin)", true)
                    .addArgument({"--new-password", "-n"}, &m_newPassword,
                                 "New password of the user", false),
            ChangePassword);
    m_commandParser.addCommand(
            Command("user_sessions",
                    "Shows all active sessions of the current user!"),
            ShowSessions);
    m_commandParser.addCommand(
            Command("user_session_revoke", "Revokes an active session!")
                    .addArgument({"--session-id", "-s"}, &m_sessionId,
                                 "Id of the session which should be revoked",
                                 false),
            RevokeSession);
}

/**
 * Dispatches an entered CLI command to the UserApi.
 *
 * Flow: the parser splits the input into a command and its parameters, then a
 * switch over the nine commands executes the matching UserApi operation and
 * prints the result. All filters (group name, virtual group) are taken from the
 * previously set session context, not from the parameters - the CLI session
 * carries them.
 *
 * Every call is wrapped in a try/catch: a failed UserApi operation should not
 * end the menu loop but show up as an error message. The handler therefore
 * does not throw upwards.
 *
 * @param userDto    the logged in user whose rights control the commands
 * @param input      the raw CLI input
 * @param parameters the already split parameters of the input
 */
void UserManagementCliComponent::onCommand(
        const UserDto &userDto, const std::string &input,
        const std::vector<std::string> &parameters) {
    try {
        Commands command = m_commandParser.parse(input, parameters);
        switch (command) {
      // groups: filters come from the session context, not from the parameters.
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
      // users: same, the same four filter combinations.
            case AllUsers: {
                if (m_userName.has_value()) {
                    printUsers(m_userApi->allUsersOf(m_userName.value()));
                } else {
                    printUsers(m_userApi->allUsersOf());
                }
                break;
            }
      // create: name, e-mail, password and groups come from the parameters,
      // the password is passed base64-encoded.
            case AddUser: {
                if (!m_firstName.has_value() || !m_lastName.has_value() ||
                    !m_eMail.has_value() || !m_userName.has_value() ||
                    !m_password.has_value()) {
                    std::cerr << "ERROR: please make sure that all arguments are "
                                 "correctly filled\n";
                    break;
                }
                User user(m_firstName.value(), m_lastName.value(), User::Sex::Male,
                          m_eMail.value(), m_userName.value(), m_password.value(), {});
                UserDto userDtoNew(user);
                userDtoNew.setPassword(Cryption::encodeBase64(m_password.value()));
                m_userApi->createOf(userDtoNew);
                break;
            }
      // update: like create, but with the target identifier from the parameter.
            case UpdateUser: {
                if (!m_userName.has_value()) {
                    std::cerr << "ERROR: please set username\n";
                    break;
                }
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
      // delete: requires a confirmation, otherwise no action.
            case RemoveUser: {
                if (!m_userName.has_value()) {
                    std::cout << "ERROR: Please add username as value\n";
                    break;
                }
                std::vector<std::string> userRemoves;
                userRemoves.push_back(m_userName.value());
                m_userApi->deleteOf(userRemoves);
                break;
            }
      // password change: old and new password from the parameters.
            case ChangePassword: {
                if (!m_userName.has_value() || !m_newPassword.has_value()) {
                    std::cerr << "ERROR: please set username and new password\n";
                    break;
                }
                UserPasswordChangeDto passwordChangeDto(
                        m_userName.value(),
                        Cryption::encodeBase64(m_oldPassword.value_or("")),
                        Cryption::encodeBase64(m_newPassword.value()));
                if (!m_userApi->changePasswordOf(passwordChangeDto)) {
                    std::cerr << "ERROR: failed to change password\n";
                }
                break;
            }
      // sessions: display only, no parameters.
            case ShowSessions:
                printSessions(m_userApi->sessionsOf());
                break;
      // revoke session: the session id is the first parameter.
            case RevokeSession: {
                if (!m_sessionId.has_value()) {
                    std::cerr << "ERROR: please set the session id\n";
                    break;
                }
                if (!m_userApi->revokeSessionOf(m_sessionId.value())) {
                    std::cerr << "ERROR: failed to revoke session\n";
                }
                break;
            }
            default:
                break;
        }
    } catch (const std::exception &exception) {
        std::cerr << "ERROR: " << exception.what() << "\n";
    }
}

void UserManagementCliComponent::onHelp() {
    m_commandParser.printHelp(getName(), getAlias(), m_description);
}

void UserManagementCliComponent::onShowMenu() {}

bool UserManagementCliComponent::onMenu(const std::string &component) {
    return true;
}

bool UserManagementCliComponent::onExit() { return true; }

void UserManagementCliComponent::printGroups(
        const std::vector<GroupDto> &groups) {
    TableBuilder<4> builder;
    builder.add({"No.", "Name", "IsVirtual", "SubGroups"});
    std::size_t iter = 0;
    for (const auto &group: groups) {
        builder.add({std::to_string(++iter), group.getGroupName(),
                     group.isVirtual() ? "true" : "false",
                     printSubGroups(group)});
    }
    std::cout << builder.build() << "\n";
}

std::string UserManagementCliComponent::printSubGroups(const GroupDto &group) {
    std::string printString;
    std::vector<GroupDto> subGroups = group.getSubGroups();
    for (std::size_t i = 0; i < subGroups.size(); i++) {
        const auto &subGroup = subGroups[i];
        printString += subGroup.getGroupName();
        if (i < subGroups.size() - 1) {
            printString += ";";
        }
    }
    return printString;
}

void UserManagementCliComponent::printUsers(const std::vector<UserDto> &users) {
    TableBuilder<6> builder;
    builder.add(
            {"No.", "firstname", "lastname", "e-mail", "username", "groups"});
    std::size_t iter = 0;
    for (const auto &user: users) {
        builder.add({std::to_string(++iter), user.getFirstName(),
                     user.getLastName(), user.getEMail(), user.getUserName(),
                     printUserGroups(user.getGroups())});
    }
    std::cout << builder.build() << "\n";
}

std::string UserManagementCliComponent::printUserGroups(
        const std::vector<GroupDto> &groups) {
    std::string printString;
    for (std::size_t i = 0; i < groups.size(); i++) {
        const auto &group = groups[i];
        printString += group.getGroupName();
        if (i < groups.size() - 1) {
            printString += ";";
        }
    }
    return printString;
}

void UserManagementCliComponent::printSessions(
        const std::vector<UserSessionDto> &sessions) {
    TableBuilder<4> builder;
    builder.add({"No.", "Id", "IP Address", "Last Access"});
    std::size_t iter = 0;
    for (const auto &session: sessions) {
        builder.add({std::to_string(++iter), session.getId(),
                     session.getIpAddress(),
                     StringifyService<date::sys_time<
                             std::chrono::microseconds>>::
                             serializeToString(
                                     session.getLastAccessTimestamps())});
    }
    std::cout << builder.build() << "\n";
}

void UserManagementCliComponent::printCommandList(std::set<std::string> menuAlias) {
    m_commandParser.printCommandList(menuAlias);
}

std::vector<std::string> UserManagementCliComponent::allCommandsOf() {
    return m_commandParser.allCommandsOf();
}
