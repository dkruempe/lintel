#ifndef CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H
#define CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H

#include <map>
#include <string>

#include "base_library/features/base/configuration/Cryption.h"
#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/cli/models/CommandLineComponent.h"

class CryptionCliComponent : public CommandLineComponent {
 private:
  enum Command {
    CommandEncrypt,
    CommandDecrypt,
    CommandUserLogin,
    CommandUserLogout,
    CommandUndefined,
  };
  std::map<std::string_view, Command> m_commands = {
      {"encrypt", CommandEncrypt},  {"enc", CommandEncrypt},
      {"decrypt", CommandDecrypt},  {"dec", CommandDecrypt},
      {"login", CommandUserLogin},  {"log", CommandUserLogin},
      {"logout", CommandUserLogout}};
  static constexpr std::string_view m_name = "Cryption";
  static constexpr std::string_view m_alias = "Crypt";
  Command m_currentCommand = CommandUndefined;
  Cryption m_cryption;
  std::shared_ptr<UserApi> m_userApi;
  std::string m_loginId;

 public:
  explicit CryptionCliComponent(std::shared_ptr<UserApi> m_userApi);

  void onCommand(const std::string &input,
                 const std::vector<std::string> &parameters) override;

  void onHelp() override;

  void onShowMenu() override;

  bool onMenu(const std::string &component) override;

  bool onExit() override;
};

#endif  // CPP_BASE_LIBRARY_CRYPTIONCLICOMPONENT_H
