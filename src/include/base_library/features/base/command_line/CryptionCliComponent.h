#ifndef CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H
#define CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H

#include <map>
#include <string>

#include "base_library/features/base/configuration/Cryption.h"
#include "base_library/features/cli/models/CommandLineComponent.h"
#include "base_library/features/property/controller/PropertyApi.h"

class CryptionCliComponent : public CommandLineComponent {
 private:
  enum Command {
    CommandEncrypt,
    CommandDecrypt,
    CommandUndefined,
  };
  std::map<std::string_view, Command> m_commands = {
      {"encrypt", CommandEncrypt},
      {"enc", CommandEncrypt},
      {"decrypt", CommandDecrypt},
      {"dec", CommandDecrypt}};
  static constexpr std::string_view m_name = "Cryption";
  static constexpr std::string_view m_alias = "Crypt";
  Command m_currentCommand = CommandUndefined;
  Cryption m_cryption;
  std::shared_ptr<PropertyApi> m_propertyApi;

 public:
  explicit CryptionCliComponent(std::shared_ptr<PropertyApi> propertyApi);

  void onCommand(const std::string &input) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
};

#endif  // CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H
