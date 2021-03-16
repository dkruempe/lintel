#include <utility>

#include "base_library/configuration/Component.h"
#include "base_library/configuration/Configuration.h"
#include "base_library/configuration/ConnectionComponent.h"
#include "base_library/configuration/ConnectionEntry.h"
#include "base_library/configuration/Cryption.h"
#include "base_library/configuration/PropertyComponent.h"
#include "base_library/configuration/PropertyEntry.h"
#include "base_library/services/LoggerService.h"
#include "base_library/utils/TypeName.h"

void encryptTest() {
  Cryption cryption;
  std::string cipherText = cryption.encryption("Dominik AAAAAAAd");
  LOG_INFO("encryption: {}", cipherText);
  std::string plainText = cryption.decryption(cipherText);
  LOG_INFO("decryption: {}", plainText);
}

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(std::filesystem::path(argv[0]).filename());
  std::shared_ptr<Component> component = std::make_shared<PropertyComponent>();
  std::shared_ptr<ConnectionComponent> environment =
      std::make_shared<ConnectionComponent>();
  Configuration configurationParser({component, environment}, "bootstrap");
  std::vector<std::shared_ptr<Entry>> properties =
      configurationParser.configurationOf<PropertyComponent>();
  for (auto &iter : properties) {
    auto propertyPtr = std::static_pointer_cast<PropertyEntry>(iter);
    std::stringstream ss;
    ss << *propertyPtr;
    LOG_INFO("{}", ss.str());
  }
  std::vector<std::shared_ptr<Entry>> environments =
      configurationParser.configurationOf<ConnectionComponent>();
  for (auto &iter : environments) {
    auto environemntPtr = std::static_pointer_cast<ConnectionEntry>(iter);
    std::stringstream ss;
    ss << *environemntPtr;
    LOG_INFO("{}", ss.str());
  }

  encryptTest();
  return 0;
}