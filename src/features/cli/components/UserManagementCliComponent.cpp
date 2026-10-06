#include "base_library/features/cli/components/UserManagementCliComponent.h"

#include <base_library/core/utils/TableBuilder.h>

#include "base_library/core/services/StringifyService.h"
#include "base_library/core/utils/Cryption.h"

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
 * Dispatcht einen eingegebenen CLI-Befehl an die UserApi.
 *
 * Ablauf: Der Parser zerlegt die Eingabe in ein Kommando und dessen Parameter,
 * danach fuehrt ein Switch ueber die neun Kommandos die passende UserApi-
 * Operation aus und druckt das Ergebnis. Alle Filter (Gruppenname,
 * virtuelle Gruppe) aus dem zuvor gesetzten Sitzungs-Kontext uebernommen,
 * nicht aus den Parametern - die CLI-Session traegt sie.
 *
 * Jeder Aufruf ist in einen try/catch gesetzt: eine fehlgeschlagene
 * UserApi-Operation soll die Menueschleife nicht beenden, sondern als
 * Fehlermeldung erscheinen. Der Handler wirft deshalb nach oben nichts.
 *
 * @param userDto    der angemeldete Benutzer, dessen Rechte die Kommandos steuern
 * @param input      die rohe CLI-Eingabe
 * @param parameters die bereits aufgeteilten Parameter der Eingabe
 */
void UserManagementCliComponent::onCommand(
        const UserDto &userDto, const std::string &input,
        const std::vector<std::string> &parameters) {
    try {
        Commands command = m_commandParser.parse(input, parameters);
        switch (command) {
      // Gruppen: Filter stammen aus dem Sitzungs-Kontext, nicht aus den Parametern.
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
      // Benutzer: dito, gleiche vier Filterkombinationen.
            case AllUsers: {
                if (m_userName.has_value()) {
                    printUsers(m_userApi->allUsersOf(m_userName.value()));
                } else {
                    printUsers(m_userApi->allUsersOf());
                }
                break;
            }
      // Anlegen: Name, EMail, Passwort und Gruppen kommen aus den Parametern,
      // das Passwort wird base64-kodiert uebergeben.
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
      // Aendern: wie Anlegen, aber mit der Zielkennung aus dem Parameter.
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
      // Loeschen: verlangt eine Bestaetigung, sonst keine Aktion.
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
      // Passwortwechsel: altes und neues Passwort aus den Parametern.
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
      // Sitzungen: nur anzeigen, keine Parameter.
            case ShowSessions:
                printSessions(m_userApi->sessionsOf());
                break;
      // Sitzung widerrufen: die Session-ID ist der erste Parameter.
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
