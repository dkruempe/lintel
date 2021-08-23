#include <iostream>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConnectionComponent.h"
#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/repositories/UserRepository.h"

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(argv[0]);
  // base initialization stuff
  std::shared_ptr<Component> connectionComponent =
      std::make_shared<ConnectionComponent>();
  std::vector<std::shared_ptr<Component>> vec{connectionComponent};
  std::shared_ptr<Configuration> configuration =
      std::make_shared<Configuration>(vec);
  std::shared_ptr<ConnectionConfigurations> connectionConfigurations =
      std::make_shared<ConnectionConfigurations>(configuration);
  UserRepository userRepository(connectionConfigurations);
  GroupRepository groupRepository(connectionConfigurations);
  std::optional<User> anna = userRepository.of("anna");
  std::vector<Group> groups = groupRepository.allOf();
  for (const auto &group : groups) {
    std::cout << group << std::endl;
  }
  std::optional<Group> propertyAdmin = groupRepository.of("Property-Admin");
  Group newGroup("test", {propertyAdmin.value()}, false);
  groupRepository.createOf(newGroup);
  std::cout << groupRepository.of("test").value() << std::endl;
  groupRepository.deleteOf(newGroup);
  auto testGroup = groupRepository.of("test");
  return 0;
}