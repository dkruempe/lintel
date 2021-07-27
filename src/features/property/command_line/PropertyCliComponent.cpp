#include "base_library/features/property/command_line/PropertyCliComponent.h"
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <tabulate/table.hpp>

#include "base_library/core/services/LoggerService.h"

PropertyCliComponent::PropertyCliComponent(
    std::shared_ptr<PropertyApi> propertyApi)
    : CommandLineComponent(m_name, m_alias),
      m_propertyApi(std::move(propertyApi)) {}

void PropertyCliComponent::onCommand(const std::string &input) {
  switch (m_currentCommand) {
    case CommandAllPropertiesOfProcess: {
      m_process = input;
      std::vector<PropertyDto> properties = m_propertyApi->allOf(m_process);
      printProperties(properties);
      m_process = "";
      m_currentCommand = CommandUndefined;
      return;
    }
    default:
      break;
  }
  auto found = m_commands.find(input);
  Command command =
      found == m_commands.end() ? CommandUndefined : found->second;
  switch (command) {
    case CommandAllProperties: {
      allOf();
    } break;
    case CommandAllPropertiesOfProcess: {
      fmt::print("Please enter the process name:\n");
      m_currentCommand = CommandAllPropertiesOfProcess;
      break;
    }
    default:
      break;
  }
}
void PropertyCliComponent::printProperties(
    const std::vector<PropertyDto> &properties) {
  tabulate::Table table;
  table.add_row({"No.", "Process", "Class", "Instance", "Name", "Type", "Value",
                 "Data Storage"});
  std::size_t iter = 0;
  for (const auto &property : properties) {
    table.add_row({std::to_string(++iter), property.getProcessName(),
                   property.getClassName(), property.getInstanceName(),
                   property.getName(), property.getType(), property.getValue(),
                   property.getRepositoryType().toString()});
  }
  fmt::print("{}\n", table.str());
}
void PropertyCliComponent::allOf() {
  const std::vector<PropertyDto> &properties = m_propertyApi->allOf();
  printProperties(properties);
}

void PropertyCliComponent::onHelp() {
  // group commands map by command enum
  std::map<Command, std::vector<std::string_view>> map;
  for (auto &iter : m_commands) {
    auto found = map.find(iter.second);
    if (found == map.end()) {
      map.insert({iter.second, {iter.first}});
    } else {
      found->second.push_back(iter.first);
    }
  }

  std::function<void(std::vector<std::string_view> &)> print =
      [&](std::vector<std::string_view> &aliases) {
        fmt::print("[");
        for (std::size_t i = 0; i < aliases.size(); i++) {
          fmt::print("{}", aliases[i]);
          if (i < aliases.size() - 1) {
            fmt::print(", ", aliases[i]);
          }
        }
        fmt::print("]\n");
      };

  fmt::print("Help Overview\n");
  for (auto &[command, aliases] : map) {
    switch (command) {
      case CommandAllProperties:
        fmt::print("CommandAllProperties: shows all properties");
        print(aliases);
        break;
      case CommandAllPropertiesOfProcess:
        fmt::print(
            "CommandAllPropertiesOfProcess: shows all properties with "
            "given process");
        print(aliases);
        break;
      default:
        fmt::print("Undefined: command not available \n");
        LOG_ERROR("undefined state");
        break;
    }
  }
}

bool PropertyCliComponent::onMenu(const std::string &component) { return true; }

void PropertyCliComponent::onShowMenu() { fmt::print("No submenu available!"); }

bool PropertyCliComponent::onExit() { return true; }
