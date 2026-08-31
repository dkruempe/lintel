#include "base_library/features/cli/components/ProcessCliComponent.h"

#include <base_library/core/utils/TableBuilder.h>

ProcessCliComponent::ProcessCliComponent(std::shared_ptr<ProcessApi> processApi)
  : CommandLineComponent(m_name, m_alias), m_processApi(std::move(processApi))
{
  m_commandParser.addCommand(
    Command("show_processes", "Show all processes!")
      .addArgument(
        { "--process-name", "-p" }, &m_processName, "Process Name of process itself, which is same as the filename."),
    ShowProcesses);
  m_commandParser.addCommand(Command("show_groups", "Show all process groups")
                               .addArgument({ "--process-group", "-g" }, &m_processGroup, "Process Group Name"),
    ShowProcessGroups);
  m_commandParser.addCommand(Command("start_process", "Start Process")
                               .addArgument({ "--process-path", "-p" }, &m_processName, "Path of Process")
                               .addArgument({ "--arguments", "-a" }, &m_arguments, "Arguments of process")
                               .addArgument({ "--restarts", "-r" }, &m_restarts, "Restarts of process"),
    StartProcess);
  m_commandParser.addCommand(
    Command("stop_process", "Stop Process").addArgument({ "--process-id", "-i" }, &m_processId, "Id of Process"),
    StopProcess);
  m_commandParser.addCommand(Command("terminate_process", "Terminate Process")
                               .addArgument({ "--process-id", "-i" }, &m_processId, "Id of Process"),
    TerminateProcess);
  m_commandParser.addCommand(
    Command("restart_process", "Restart Process").addArgument({ "--process-id", "-i" }, &m_processId, "Id of Process"),
    RestartProcess);
  m_commandParser.addCommand(Command("reset_process", "Reset restarts and failure state of Process")
                               .addArgument({ "--process-id", "-i" }, &m_processId, "Id of Process"),
    ResetProcess);
  m_commandParser.addCommand(Command("show_process_details", "Show resource details of Process")
                               .addArgument({ "--process-id", "-i" }, &m_processId, "Id of Process"),
    ShowProcessDetails);
}

void ProcessCliComponent::onCommand(const UserDto & /*userDto*/,
  const std::string &input,
  const std::vector<std::string> &parameters)
{
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
      std::shared_ptr<Process> process = std::make_shared<Process>(path, m_arguments);
      if (m_restarts.has_value() && m_restarts.value() > 0) { process->enableAutoStart(m_restarts.value()); }
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
    case RestartProcess: {
      if (m_processId.empty()) {
        std::cout << "ERROR: please enter valid process id\n";
        return;
      }
      m_processApi->restartOf(m_processId);
      break;
    }
    case ResetProcess: {
      if (m_processId.empty()) {
        std::cout << "ERROR: please enter valid process id\n";
        return;
      }
      m_processApi->resetOf(m_processId);
      break;
    }
    case ShowProcessDetails: {
      if (m_processId.empty()) {
        std::cout << "ERROR: please enter valid process id\n";
        return;
      }
      auto info = m_processApi->healthOf(m_processId);
      if (!info.has_value()) {
        std::cout << "ERROR: no details available for process " << m_processId << "\n";
        return;
      }
      printProcessDetails(info.value());
      break;
    }
    default:
      break;
    }
  } catch (std::exception &exception) {
    std::cerr << "ERROR: " << exception.what() << "\n";
  }
}

void ProcessCliComponent::printProcessGroups(const std::vector<ProcessGroupDto> &processGroup)
{
  TableBuilder<3> builder;
  builder.add({ "No.", "ProcessGroupName", "ProcessGroupId" });
  std::size_t count = 0;
  for (const auto &iter : processGroup) { builder.add({ std::to_string(++count), iter.getName(), iter.getId() }); }
  std::cout << builder.build() << "\n";
}

void ProcessCliComponent::printProcesses(const std::vector<ProcessInfoDto> &processInfoDto)
{
  TableBuilder<9> builder;
  builder.add({
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
    builder.add({ std::to_string(++iter),
      processInfo.getPath().filename().string(),
      std::to_string(processInfo.getProcessId()),
      processInfo.getId(),
      processInfo.isAutoRestart() ? "true" : "false",
      std::to_string(processInfo.getMaxAutoRestarts()),
      std::to_string(processInfo.getRestarts()),
      processInfo.getGroupName(),
      processInfo.getGroupId() });
  }
  std::cout << builder.build() << "\n";
}

bool ProcessCliComponent::onExit() { return true; }

void ProcessCliComponent::printProcessDetails(const ProcessInfoDto &processInfo)
{
  TableBuilder<2> builder;
  builder.add({ "Key", "Value" });
  builder.add({ "ProcessId", processInfo.getId() });
  builder.add({ "ProcessName", processInfo.getPath().filename().string() });
  builder.add({ "SystemProcessId", std::to_string(processInfo.getProcessId()) });
  builder.add({ "AutoRestart", processInfo.isAutoRestart() ? "true" : "false" });
  builder.add({ "MaxRestarts", std::to_string(processInfo.getMaxAutoRestarts()) });
  builder.add({ "Restarts", std::to_string(processInfo.getRestarts()) });
  builder.add({ "Running", processInfo.isRunning() ? "true" : "false" });
  builder.add({ "ExitCode", std::to_string(processInfo.getExitCode()) });
  builder.add({ "ExitCodeValid", processInfo.isExitCodeValid() ? "true" : "false" });
  builder.add({ "GroupName", processInfo.getGroupName() });
  builder.add({ "GroupId", processInfo.getGroupId() });
  builder.add({ "ResourcesValid", processInfo.isResourceDataValid() ? "true" : "false" });
  if (processInfo.getUptimeSeconds().has_value()) {
    builder.add({ "UptimeSeconds", std::to_string(processInfo.getUptimeSeconds().value()) });
  }
  if (processInfo.getCpuPercent().has_value()) {
    builder.add({ "CpuPercent", std::to_string(processInfo.getCpuPercent().value()) });
  }
  if (processInfo.getMemoryBytes().has_value()) {
    builder.add({ "MemoryBytes", std::to_string(processInfo.getMemoryBytes().value()) });
  }
  std::cout << builder.build() << "\n";
}

void ProcessCliComponent::printCommandList(std::set<std::string> menuAlias)
{ m_commandParser.printCommandList(menuAlias); }

void ProcessCliComponent::onShowMenu() { std::cout << "No subMenu available\n"; }

bool ProcessCliComponent::onMenu(const std::string & /*component*/) { return true; }

void ProcessCliComponent::onHelp() { m_commandParser.printHelp(getName(), getAlias(), m_description); }

std::vector<std::string> ProcessCliComponent::allCommandsOf() { return m_commandParser.allCommandsOf(); }
