#include "base_library/features/cli/models/AbstractCommandLineMenu.h"

#include <tabulate/table.hpp>

#include "base_library/features/cli/models/CommandLineComponent.h"

AbstractCommandLineMenu::AbstractCommandLineMenu(
    const std::vector<std::shared_ptr<CommandLineComponent>> &components)
    : m_components(components), m_componentMap(build(components)) {}

std::map<std::string_view, std::shared_ptr<CommandLineComponent>>
AbstractCommandLineMenu::build(
    const std::vector<std::shared_ptr<CommandLineComponent>> &menuEntries) {
  std::map<std::string_view, std::shared_ptr<CommandLineComponent>> map;
  for (auto &menuEntry : menuEntries) {
    map.insert({menuEntry->getName(), menuEntry});
    map.insert({menuEntry->getAlias(), menuEntry});
  }
  return map;
}

void AbstractCommandLineMenu::onShowMenu() {
  if (m_current != nullptr) {
    m_current->onShowMenu();
    return;
  }
  int index = 0;
  tabulate::Table table;
  table.add_row({"Index", "Menu Name", "Menu Alias"});
  for (auto &component : m_components) {
    table.add_row({std::to_string(++index), std::string(component->getName()),
                   std::string(component->getAlias())});
  }
  std::cout << table.str() << "\n";
}

bool AbstractCommandLineMenu::onMenu(const std::string &command) {
  if (m_current != nullptr) {
    return m_current->onMenu(command);
  }

  auto found = m_componentMap.find(command);
  if (found == m_componentMap.end()) {
    return false;
  }
  m_current = found->second;
  return true;
}
bool AbstractCommandLineMenu::onExit() {
  bool success = m_current->onExit();
  if (m_current != nullptr) {
    m_current = nullptr;
  }
  return success;
}
void AbstractCommandLineMenu::onCommand(
    const UserDto &userDto, const std::string &command,
    const std::vector<std::string> &parameters) {
  if (m_current == nullptr) {
    return;
  }
  m_current->onCommand(userDto, command, parameters);
}
void AbstractCommandLineMenu::onHelp() {
  if (m_current == nullptr) {
    return;
  }
  m_current->onHelp();
}
const std::shared_ptr<CommandLineComponent>
    &AbstractCommandLineMenu::currentOf() {
  return m_current;
}
void AbstractCommandLineMenu::printCommandList() {
  if (m_current == nullptr) {
    return;
  }
  m_current->printCommandList();
}