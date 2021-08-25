#include <sstream>

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
  std::shared_ptr<GroupRepository> groupRepository =
      std::make_shared<GroupRepository>(connectionConfigurations);
  UserRepository userRepository(connectionConfigurations, groupRepository);
  std::optional<User> dkruempe = userRepository.of("dkruempe");
  {
    std::stringstream ss;
    ss << dkruempe.value();
    LOG_INFO("{}", ss.str());
  }
  std::vector<Group> groups = groupRepository->allOf();
  for (const auto &group : groups) {
    std::stringstream ss;
    ss << group;
    LOG_INFO("{}", ss.str());
  }
  std::optional<Group> propertyAdmin = groupRepository->of("Property-Admin");
  Group newGroup("test", {propertyAdmin.value()}, false);
  groupRepository->createOf(newGroup);
  {
    std::stringstream ss;
    ss << groupRepository->of("test").value();
    LOG_INFO("{}", ss.str());
  }
  groupRepository->deleteOf(newGroup);
  auto testGroup = groupRepository->of("test");
  User user("Josef", "Hermes", User::Sex::Male, "josef.hermes@aol.com", "jopp",
            "jopp", {});
  userRepository.createOf(user);
  for (const auto &user : userRepository.allOf()) {
    std::stringstream ss;
    ss << user;
    LOG_INFO("{}", ss.str());
  }
  userRepository.deleteOf(user);
  return 0;
}