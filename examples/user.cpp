#include <sstream>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConnectionComponent.h"
#include "base_library/features/base/configuration/Cryption.h"
#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/repositories/UserRepository.h"

void testUser() {
  // base initialization stuff
  std::shared_ptr<Component> connectionComponent =
      std::make_shared<ConnectionComponent>();
  std::vector<std::shared_ptr<Component>> vec{connectionComponent};
  std::shared_ptr<Configuration> configuration =
      std::make_shared<Configuration>(vec);
  std::shared_ptr<ConnectionConfigurations> connectionConfigurations =
      std::make_shared<ConnectionConfigurations>(configuration);
  std::shared_ptr<GroupRepository> groupRepository =
      std::make_shared<GroupRepository>(connectionConfigurations);
  UserRepository userRepository(connectionConfigurations, groupRepository);
  User user("Dominik", "Krümpelmann", User::Sex::Male, "example@example.com",
            "dkruempe", Cryption::hashOf("${ADMIN_PASSWORD}"), {});
  userRepository.createOf(user);
}

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(argv[0]);
  testUser();
  return 0;
}