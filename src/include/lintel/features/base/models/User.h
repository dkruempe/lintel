#ifndef LINTEL_USER_H
#define LINTEL_USER_H

#include <date/date.h>

#include <chrono>
#include <ostream>
#include <set>
#include <string>
#include <utility>

#include "lintel/features/base/models/Group.h"

/**
 * User class for controlling of access right etc.
 *
 * The user class contains all important information for the user itself.
 * Also the a collection of groups, which implies the access right for the user.
 *
 * All information of the user are saved in the database and the password is
 * of course saved as password hash.
 */
class User {
public:
    enum Sex {
        Male, Female
    };

private:
    std::string m_firstName;
    std::string m_lastName;
    Sex m_sex;
    std::string m_email;
    std::string m_userName;
    std::string m_password;

    date::sys_time<std::chrono::microseconds> m_createdTimestamp;
    std::vector<Group> m_groups;
    std::set<Group> m_allGroups{};  // including subgroups

public:
    /**
     * Constructor.
     * @param firstName user's first name
     * @param lastName user's last name
     * @param sex user's sex
     * @param email user's email address
     * @param userName login username
     * @param password password (will be hashed)
     * @param groups groups this user belongs to
     * @param createdTimestamp creation timestamp (defaults to now)
     */
    User(std::string firstName, std::string lastName, Sex sex, std::string email,
         std::string userName, std::string password, std::vector<Group> groups,
         date::sys_time<std::chrono::microseconds> createdTimestamp =
         std::chrono::time_point_cast<std::chrono::microseconds>(
                 std::chrono::system_clock::now()));

    /** @return first name */
    [[nodiscard]] const std::string &getFirstName() const;

    /** @return last name */
    [[nodiscard]] const std::string &getLastName() const;

    /** @return sex */
    [[nodiscard]] Sex getSex() const;

    /** @return email address */
    [[nodiscard]] const std::string &getEmail() const;

    /** @return login username */
    [[nodiscard]] const std::string &getUserName() const;

    /** @return password hash */
    [[nodiscard]] const std::string &getPassword() const;

    /**
     * Check if the user belongs to the specified group (including subgroups).
     * @param group group to check
     * @return true if the user has the group
     */
    bool has(const Group &group) const;

    /** @return creation timestamp */
    [[nodiscard]] const date::sys_time<std::chrono::microseconds>
    &getCreatedTimestamp() const;

    /** @return direct groups assigned to the user */
    [[nodiscard]] const std::vector<Group> &getGroups() const;

    friend std::ostream &operator<<(std::ostream &os, const User &user);
};

#endif  // LINTEL_USER_H
