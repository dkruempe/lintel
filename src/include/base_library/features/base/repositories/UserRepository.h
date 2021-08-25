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

  void createOf(const User &user);

  void deleteOf(const User &user);

  void updateOf(const User &user);
};

#endif  // CPP_BASE_LIBRARY_USERREPOSITORY_H
