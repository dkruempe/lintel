#ifndef CPP_BASE_LIBRARY_USERREPOSITORY_H
#define CPP_BASE_LIBRARY_USERREPOSITORY_H

#include <optional>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"
#include "base_library/features/base/models/User.h"
#include "base_library/features/base/repositories/GroupRepository.h"

class UserRepository {
 private:
  std::shared_ptr<ConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<ConnectionEntry> m_connectionEntry;
  std::shared_ptr<GroupRepository> m_groupRepository;

 public:
  UserRepository(
      std::shared_ptr<ConnectionConfigurations> connectionConfigurations,
      std::shared_ptr<GroupRepository> groupRepository);

  std::optional<User> of(const std::string &userName);

  std::vector<User> allOf();

  void createOf(const User &user);

  void deleteOf(const User &user);

  void addGroupOf(const User &user, const Group &group);

  void removeGroupOf(const User &user, const Group &group);

  void changeFirstNameOf(const User &user, const std::string &firstName);

  void changeLastNameOf(const User &user, const std::string &lastName);

  void changeUserNameOf(const User &user, const std::string &userName);

  void changePasswordOf(const User &user, const std::string &password);

  void changeEMailOf(const User &user, const std::string &eMail);
};

#endif  // CPP_BASE_LIBRARY_USERREPOSITORY_H
