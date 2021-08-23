#ifndef CPP_BASE_LIBRARY_USERREPOSITORY_H
#define CPP_BASE_LIBRARY_USERREPOSITORY_H

#include <optional>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"
#include "base_library/features/base/models/User.h"

class UserRepository {
 private:
  std::shared_ptr<ConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<ConnectionEntry> m_connectionEntry;

 public:
  explicit UserRepository(
      std::shared_ptr<ConnectionConfigurations> connectionConfigurations);

  std::optional<User> of(const std::string &userName);

  void createOf(const User &user);

  void deleteOf(const User &user);

  void updateOf(const User &user);
};

#endif  // CPP_BASE_LIBRARY_USERREPOSITORY_H
