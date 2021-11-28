#include "base_library/features/base/command_line/ProcessCliComponent.h"

#include <tabulate/table.hpp>
ProcessCliComponent::ProcessCliComponent(std::shared_ptr<ProcessApi> processApi)
    : CommandLineComponent(m_name, m_alias),
      m_processApi(std::move(processApi)) {
  m_commandParser.addCommand(
      Command("show_processes", "Show all processes!")
          .addArgument(
              {"-process-name", "-p"}, &m_processName,
              "Process Name of process itself, which is same as the filename."),
      ShowProcesses);
}
void ProcessCliComponent::onCommand(
    const UserDto & /*userDto*/, const std::string &input,
    const std::vector<std::string> &parameters) {
  try {
    Commands command = m_commandParser.parse(input, parameters);
    switch (command) {
      case ShowProcesses: {
        std::vector<ProcessInfoDto> temp{};
        if (m_processName.has_value()) {
          temp = m_processApi->allOf(m_processName.value());
        } else {
          temp = m_processApi->allOf(".*");
        }
        printProcesses(temp);
        break;
      }
      case Undefined: {
        std::cout << "Undefined command >" << input << "<\n";
        break;
      }
    }
  } catch (std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}
void ProcessCliComponent::printProcesses(
    const std::vector<ProcessInfoDto> &processInfoDto) {
  tabulate::Table table;
  table.add_row({
      "No.",
      "ProcessName",
      "SystemProcessId",
      "ProcessId",
      "AutoRestart",
      "MaxRestarts",
      "Restarts",
      "GroupName",
      "GroupId",
  });
  std::size_t iter = 0;
  for (const auto &processInfo : processInfoDto) {
    table.add_row(
        {std::to_string(++iter), processInfo.getPath().filename().string(),
         std::to_string(processInfo.getProcessId()), processInfo.getId(),
         processInfo.isAutoRestart() ? "true" : "false",
         std::to_string(processInfo.getRestarts()),
         std::to_string(processInfo.getMaxAutoRestarts()),
         processInfo.getGroupName(), processInfo.getGroupId()});
  }
  std::cout << table.str() << "\n";
}
bool ProcessCliComponent::onExit() { return true; }
void ProcessCliComponent::printCommandList() {
  m_commandParser.printCommandList();
}
void ProcessCliComponent::onShowMenu() {
  std::cout << "No subMenu available\n";
}
bool ProcessCliComponent::onMenu(const std::string & /*component*/) {
  return true;
}
void ProcessCliComponent::onHelp() {
  m_commandParser.printHelp(getName(), getAlias(), m_description);
}
