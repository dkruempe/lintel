#ifndef CPP_BASE_LIBRARY_ABSTRACTCOMMANDLINEMENU_H
#define CPP_BASE_LIBRARY_ABSTRACTCOMMANDLINEMENU_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base_library/features/cli/models/CommandLineComponent.h"

class AbstractCommandLineMenu {
 private:
  std::vector<std::shared_ptr<CommandLineComponent>> m_components;
  std::map<std::string_view, std::shared_ptr<CommandLineComponent>>
      m_componentMap;
  std::shared_ptr<CommandLineComponent> m_current = nullptr;

  static std::map<std::string_view, std::shared_ptr<CommandLineComponent>>
  build(const std::vector<std::shared_ptr<CommandLineComponent>> &components);

 public:
  explicit AbstractCommandLineMenu(
      const std::vector<std::shared_ptr<CommandLineComponent>> &components);

  void onShowMenu();

  bool onMenu(const std::string &command);

  bool onExit();

  void onCommand(const UserDto &userDto, const std::string &command,
                 const std::vector<std::string> &parameters);

  void onHelp();

  const std::shared_ptr<CommandLineComponent> &currentOf();
};

#endif  // CPP_BASE_LIBRARY_ABSTRACTCOMMANDLINEMENU_H
