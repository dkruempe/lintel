#ifndef CPP_BASE_LIBRARY_USERREPOSITORY_H
#define CPP_BASE_LIBRARY_USERREPOSITORY_H

#include <optional>
#include <set>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/base/models/User.h"
#include "base_library/features/base/repositories/GroupRepository.h"

/**
 * Repository for CRUD operations on User entities stored in the database.
 */
class UserRepository {
private:
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    std::shared_ptr<GroupRepository> m_groupRepository;

public:
    /**
     * Constructor.
     * @param connectionConfigurations database connection configuration
     * @param groupRepository group repository for resolving group references
     */
    UserRepository(std::shared_ptr<DatabaseConnectionConfigurations>
                   connectionConfigurations,
                   std::shared_ptr<GroupRepository> groupRepository);

    /**
     * Find a user by username.
     * @param userName the username
     * @return the user if found
     */
    std::optional<User> of(const std::string &userName);

    /** @return all users */
    std::vector<User> allOf();

    /**
     * @param userNameMatches filter by username (partial match)
     * @return matching users
     */
    std::vector<User> allOf(const std::string &userNameMatches);

    /**
     * Create a new user.
     * @param user user to create
     */
    void createOf(const User &user);

    /**
     * Delete a user.
     * @param user user to delete
     */
    void deleteOf(const User &user);

    /**
     * Delete users by usernames.
     * @param userNames list of usernames to delete
     */
    void deleteOf(const std::vector<std::string> &userNames);

    /**
     * Add multiple groups to a user.
     * @param user target user
     * @param groups groups to add
     */
    void addGroupsOf(const User &user, const std::set<Group> &groups);

    /**
     * Add a single group to a user.
     * @param user target user
     * @param group group to add
     */
    void addGroupOf(const User &user, const Group &group);

    /**
     * Remove multiple groups from a user.
     * @param user target user
     * @param groups groups to remove
     */
    void removeGroupsOf(const User &user, const std::set<Group> &groups);

    /**
     * Remove a single group from a user.
     * @param user target user
     * @param group group to remove
     */
    void removeGroupOf(const User &user, const Group &group);

    /**
     * Change a user's first name.
     * @param user target user
     * @param firstName new first name
     */
    void changeFirstNameOf(const User &user, const std::string &firstName);

    /**
     * Change a user's last name.
     * @param user target user
     * @param lastName new last name
     */
    void changeLastNameOf(const User &user, const std::string &lastName);

    /**
     * Change a user's username.
     * @param user target user
     * @param userName new username
     */
    void changeUserNameOf(const User &user, const std::string &userName);

    /**
     * Change a user's password.
     * @param user target user
     * @param password new password
     */
    void changePasswordOf(const User &user, const std::string &password);

    /**
     * Change a user's email address.
     * @param user target user
     * @param eMail new email address
     */
    void changeEMailOf(const User &user, const std::string &eMail);
};

#endif  // CPP_BASE_LIBRARY_USERREPOSITORY_H
