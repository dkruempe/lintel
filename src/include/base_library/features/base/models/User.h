#ifndef CPP_BASE_LIBRARY_USER_H
#define CPP_BASE_LIBRARY_USER_H

#include <date/date.h>
#include <date/tz.h>

#include <chrono>
#include <ostream>
#include <set>
#include <string>
#include <utility>

#include "base_library/features/base/models/Group.h"

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
    User(std::string firstName, std::string lastName, Sex sex, std::string email,
         std::string userName, std::string password, std::vector<Group> groups,
         date::sys_time<std::chrono::microseconds> createdTimestamp =
         std::chrono::time_point_cast<std::chrono::microseconds>(
                 std::chrono::system_clock::now()));

    [[nodiscard]] const std::string &getFirstName() const;

    [[nodiscard]] const std::string &getLastName() const;

    [[nodiscard]] Sex getSex() const;

    [[nodiscard]] const std::string &getEmail() const;

    [[nodiscard]] const std::string &getUserName() const;

    [[nodiscard]] const std::string &getPassword() const;

    bool has(const Group &group) const;

    [[nodiscard]] const date::sys_time<std::chrono::microseconds>
    &getCreatedTimestamp() const;

    [[nodiscard]] const std::vector<Group> &getGroups() const;

    friend std::ostream &operator<<(std::ostream &os, const User &user);
};

#endif  // CPP_BASE_LIBRARY_USER_H
