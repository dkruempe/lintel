#include "lintel/features/cli/models/AbstractCommandLineMenu.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include <lintel/core/utils/TableBuilder.h>

#include "lintel/core/services/LoggerService.h"
#include "lintel/features/cli/models/CommandLineComponent.h"

AbstractCommandLineMenu::AbstractCommandLineMenu(
  const std::vector<std::shared_ptr<CommandLineComponent> > &components)
  : m_components(components), m_componentMap(build(components)) {}

std::map<std::string_view, std::shared_ptr<CommandLineComponent> >
  AbstractCommandLineMenu::build(
    const std::vector<std::shared_ptr<CommandLineComponent> > &components)
{
  std::map<std::string_view, std::shared_ptr<CommandLineComponent> > map;
  for (const auto &menuEntry : components) {
    map.insert({ menuEntry->getName(), menuEntry });
    map.insert({ menuEntry->getAlias(), menuEntry });
  }
  return map;
}

void AbstractCommandLineMenu::onShowMenu()
{
  if (m_current != nullptr) {
    m_current->onShowMenu();
    return;
  }
  int index = 0;
  TableBuilder<3> builder;
  builder.add({ "Index", "Menu Name", "Menu Alias" });
  for (auto &component : m_components) {
    builder.add({ std::to_string(++index), std::string(component->getName()),
                  std::string(component->getAlias()) });
  }
  std::cout << builder.build() << "\n";
}

bool AbstractCommandLineMenu::onMenu(const std::string &command)
{
  if (m_current != nullptr) { return m_current->onMenu(command); }

  auto found = m_componentMap.find(command);
  if (found == m_componentMap.end()) { return false; }
  m_current = found->second;
  return true;
}

bool AbstractCommandLineMenu::onExit()
{
  bool success = m_current->onExit();
  m_current = nullptr;
  return success;
}

void AbstractCommandLineMenu::onCommand(
  const UserDto &userDto,
  const std::string &command,
  const std::vector<std::string> &parameters)
{
  if (m_current == nullptr) { return; }
  m_current->onCommand(userDto, command, parameters);
}

void AbstractCommandLineMenu::onHelp()
{
  if (m_current == nullptr) { return; }
  m_current->onHelp();
}

const std::shared_ptr<CommandLineComponent>
  &AbstractCommandLineMenu::currentOf() { return m_current; }

std::vector<std::string> AbstractCommandLineMenu::allCommandsOf()
{
  std::set<std::string> tempSet;
  for (const auto &[str, component] : m_componentMap) { tempSet.insert(std::string(component->getName())); }
  std::vector<std::string> temp(tempSet.begin(), tempSet.end());
  if (m_current == nullptr) { return temp; }
  std::vector<std::string> commands = m_current->allCommandsOf();
  std::transform(commands.begin(),
    commands.end(),
    std::back_inserter(temp),
    [](const std::string &command) { return command; });
  temp.emplace_back("exit");
  return temp;
}

std::set<std::string> AbstractCommandLineMenu::allMenuEntriesOf()
{
  if (m_current != nullptr) { return m_current->menuEntriesOf(); }
  std::set<std::string> temp;
  for (const auto &[str, component] : m_componentMap) { temp.insert(std::string(component->getName())); }
  return temp;
}

void AbstractCommandLineMenu::printCommandList()
{
  if (m_current == nullptr) { return; }
  auto temp = allMenuEntriesOf();
  m_current->printCommandList(temp);
}