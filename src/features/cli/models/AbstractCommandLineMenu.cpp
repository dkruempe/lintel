#include "base_library/features/cli/models/AbstractCommandLineMenu.h"
#define FMT_HEADER_ONLY
#include <fmt/format.h>

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

  fmt::print("Menu Overview\n");
  int index = 0;
  for (auto &component : m_components) {
    fmt::print("{}) {} [{}]\n",
               ++index, component->getName(), component->getAlias());
  }
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
void AbstractCommandLineMenu::onCommand(const std::string &command) {
  if (m_current == nullptr) {
    return;
  }
  m_current->onCommand(command);
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