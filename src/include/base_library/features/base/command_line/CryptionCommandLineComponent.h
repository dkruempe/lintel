#ifndef CPP_BASE_LIBRARY_CRYPTIONCOMMANDLINECOMPONENT_H
#define CPP_BASE_LIBRARY_CRYPTIONCOMMANDLINECOMPONENT_H

#include <map>
#include <string>

#include "base_library/features/base/configuration/Cryption.h"
#include "base_library/features/cli/models/CommandLineComponent.h"

class CryptionComamndLineComponent : public CommandLineComponent {
 private:
  enum COMMAND { COMMAND_ENCRYPT, COMMAND_DECRYPT, COMMAND_UNDEFINED };
  std::map<std::string_view, COMMAND> m_commands = {{"encrypt", COMMAND_ENCRYPT},
                                                    {"enc", COMMAND_ENCRYPT},
                                                    {"decrypt", COMMAND_DECRYPT},
                                                    {"dec", COMMAND_DECRYPT}};
  static constexpr std::string_view m_name = "Cryption";
  static constexpr std::string_view m_alias = "Crypt";
  COMMAND m_currentCommand = COMMAND_UNDEFINED;
  Cryption m_cryption;

 public:
  CryptionComamndLineComponent();

  void onCommand(const std::string &input) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
};

#endif  // CPP_BASE_LIBRARY_CRYPTIONCOMMANDLINECOMPONENT_H
