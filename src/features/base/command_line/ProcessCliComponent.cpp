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
  m_commandParser.addCommand(
      Command("show_process_groups", "Show all process groups")
          .addArgument({"--process-group", "-g"}, &m_processGroup,
                       "Process Group Name"),
      ShowProcessGroups);
  m_commandParser.addCommand(
      Command("start_process", "Start Process")
          .addArgument({"--process-path", "-p"}, &m_processName,
                       "Path of Process")
          .addArgument({"--arguments", "-a"}, &m_arguments,
                       "Arguments of process")
          .addArgument({"--restarts", "-r"}, &m_restarts,
                       "Restarts of process"),
      StartProcess);
  m_commandParser.addCommand(
      Command("stop_process", "Stop Process")
          .addArgument({"--process-id", "-i"}, &m_processId, "Id of Process"),
      StopProcess);
  m_commandParser.addCommand(
      Command("terminate_process", "Terminate Process")
          .addArgument({"--process-id", "-i"}, &m_processId, "Id of Process"),
      TerminateProcess);
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
      case ShowProcessGroups: {
        std::vector<ProcessGroupDto> temp{};
        if (m_processGroup.has_value()) {
          temp = m_processApi->allGroupsOf(m_processGroup.value());
        } else {
          temp = m_processApi->allGroupsOf(".*");
        }
        printProcessGroups(temp);
        break;
      }
      case StartProcess: {
        if (!m_processName.has_value()) {
          std::cout << "ERROR: please enter path\n";
          return;
        }
        std::filesystem::path path(m_processName.value());
        std::shared_ptr<Process> process =
            std::make_shared<Process>(path, m_arguments);
        if (m_restarts.has_value() && m_restarts.value() > 0) {
          process->enableAutoStart(m_restarts.value());
        }
        m_processApi->startOf(process);
        m_arguments.clear();
        break;
      }
      case StopProcess: {
        if (m_processId.empty()) {
          std::cout << "ERROR: please enter valid process id\n";
          return;
        }
        m_processApi->stopOf(m_processId);
        break;
      }
      case TerminateProcess: {
        if (m_processId.empty()) {
          std::cout << "ERROR: pleqase enter valid process id \n";
          return;
        }
        m_processApi->terminateOf(m_processId);
        break;
      }
      default:
        break;
    }
  } catch (std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}
void ProcessCliComponent::printProcessGroups(
    const std::vector<ProcessGroupDto> &processGroup) {
  tabulate::Table table;
  table.add_row({"No.", "ProcessGroupName", "ProcessGroupId"});
  std::size_t count = 0;
  for (const auto &iter : processGroup) {
    table.add_row({std::to_string(++count), iter.getName(), iter.getId()});
  }
  std::cout << table.str() << "\n";
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
         std::to_string(processInfo.getMaxAutoRestarts()),
         std::to_string(processInfo.getRestarts()), processInfo.getGroupName(),
         processInfo.getGroupId()});
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
