#include "lintel/features/base/controller/ProcessInfoDto.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/base/models/ProcessInfo.h"

namespace lintel {
ProcessInfoDto::Shapes ProcessInfoDto::m_shape{};

ProcessInfoDto::ProcessInfoDto(const ProcessInfo &processInfo)
  : m_id(processInfo.getProcess()->getId()), m_path(processInfo.getProcess()->getPath()),
    m_args(processInfo.getProcess()->getArgs()), m_autoRestart(processInfo.getProcess()->isAutoRestart()),
    m_restarts(processInfo.getProcess()->currentRestarts()),
    m_maxAutoRestarts(processInfo.getProcess()->getMaxAutoRestarts()), m_processId(processInfo.getProcessId()),
    m_isRunning(processInfo.isRunning()), m_exitCode(processInfo.getExitCode()),
    m_exitCodeValid(processInfo.isExitCodeValid()), m_groupName(processInfo.getGroupName()),
    m_groupId(processInfo.getGroupId())
{
  const std::optional<ProcessResourceData> &resourceData = processInfo.getResourceData();
  if (resourceData.has_value() && resourceData->valid) {
    m_resourceDataValid = true;
    m_uptimeSeconds = resourceData->uptime.count();
    m_cpuPercent = resourceData->cpuPercent;
    m_memoryBytes = resourceData->memoryBytes;
  }
}

const std::string &ProcessInfoDto::getId() const { return m_id; }

const std::filesystem::path &ProcessInfoDto::getPath() const { return m_path; }

const std::vector<std::string> &ProcessInfoDto::getArgs() const { return m_args; }

bool ProcessInfoDto::isAutoRestart() const { return m_autoRestart; }

int32_t ProcessInfoDto::getRestarts() const { return m_restarts; }

int32_t ProcessInfoDto::getMaxAutoRestarts() const { return m_maxAutoRestarts; }

lintel::ProcessId ProcessInfoDto::getProcessId() const { return m_processId; }

bool ProcessInfoDto::isRunning() const { return m_isRunning; }

int32_t ProcessInfoDto::getExitCode() const { return m_exitCode; }

bool ProcessInfoDto::isExitCodeValid() const { return m_exitCodeValid; }

const std::string &ProcessInfoDto::getGroupName() const { return m_groupName; }

const std::string &ProcessInfoDto::getGroupId() const { return m_groupId; }

std::optional<int64_t> ProcessInfoDto::getUptimeSeconds() const { return m_uptimeSeconds; }

std::optional<double> ProcessInfoDto::getCpuPercent() const { return m_cpuPercent; }

std::optional<uint64_t> ProcessInfoDto::getMemoryBytes() const { return m_memoryBytes; }

bool ProcessInfoDto::isResourceDataValid() const { return m_resourceDataValid; }

void ProcessInfoDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // ID
  writer->String(m_shape.ID);
  writer->String(m_id.c_str());
  // PATH
  writer->String(m_shape.PATH);
  writer->String(m_path.string().c_str());
  // ARGS
  writer->String(m_shape.ARGS);
  writer->StartArray();
  for (const auto &arg : m_args) {
    writer->StartObject();
    writer->String(m_shape.ARG);
    writer->String(arg.c_str());
    writer->EndObject();
  }
  writer->EndArray();
  // AUTO_RESTART
  writer->String(m_shape.AUTO_RESTART);
  writer->Bool(m_autoRestart);
  // RESTARTs
  writer->String(m_shape.RESTARTS);
  writer->Int(m_restarts);
  // MAX_RESTARTS
  writer->String(m_shape.MAX_RESTARTS);
  writer->Int(m_maxAutoRestarts);
  // PROCESS_ID
  writer->String(m_shape.PROCESS_ID);
  writer->Int(static_cast<int32_t>(m_processId));
  // RUNS
  writer->String(m_shape.RUNS);
  writer->Bool(m_isRunning);
  // EXIT_CODE
  writer->String(m_shape.EXIT_CODE);
  writer->Int(m_exitCode);
  // EXIT_CODE_VALID
  writer->String(m_shape.EXIT_CODE_VALID);
  writer->Bool(m_exitCodeValid);
  // GROUP_NAME
  writer->String(m_shape.GROUP_NAME);
  writer->String(m_groupName.c_str());
  // GROUP_ID
  writer->String(m_shape.GROUP_ID);
  writer->String(m_groupId.c_str());
  // resource data
  writer->String(m_shape.RESOURCES_VALID);
  writer->Bool(m_resourceDataValid);
  writer->String(m_shape.UPTIME_SECONDS);
  if (m_uptimeSeconds.has_value()) {
    writer->Int64(m_uptimeSeconds.value());
  } else {
    writer->Null();
  }
  writer->String(m_shape.CPU_PERCENT);
  if (m_cpuPercent.has_value()) {
    writer->Double(m_cpuPercent.value());
  } else {
    writer->Null();
  }
  writer->String(m_shape.MEMORY_BYTES);
  if (m_memoryBytes.has_value()) {
    writer->Uint64(m_memoryBytes.value());
  } else {
    writer->Null();
  }
  writer->EndObject();
}

bool ProcessInfoDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  // ID
  if (!json::getString(obj, m_shape.ID, m_id)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.ID);
  }
  // PATH
  std::string pathStr;
  if (json::getString(obj, m_shape.PATH, pathStr)) {
    m_path = std::filesystem::path(pathStr);
  } else {
    success = false;
    LOG_ERROR("{} not defined in json seralization", m_shape.PATH);
  }
  // ARGS
  if (const auto *args = json::member(obj, m_shape.ARGS); args != nullptr && args->IsArray()) {
    for (const auto &arg : args->GetArray()) {
      std::string value;
      if (json::getString(arg, m_shape.ARG, value)) { m_args.emplace_back(std::move(value)); }
    }
  } else {
    LOG_WARN("{} not defined in json serilization", m_shape.ARGS);
  }
  // AUTO_RESTART
  if (!json::getBool(obj, m_shape.AUTO_RESTART, m_autoRestart)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.AUTO_RESTART);
  }
  // RESTARTS
  if (!json::getInt(obj, m_shape.RESTARTS, m_restarts)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.RESTARTS);
  }
  // MAX_RESTARTS
  if (!json::getInt(obj, m_shape.MAX_RESTARTS, m_maxAutoRestarts)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.MAX_RESTARTS);
  }
  // PROCESS_ID
  std::int32_t processId = 0;
  if (json::getInt(obj, m_shape.PROCESS_ID, processId)) {
    m_processId = processId;
  } else {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PROCESS_ID);
  }
  // RUNS
  if (!json::getBool(obj, m_shape.RUNS, m_isRunning)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.RUNS);
  }
  // EXIT_CODE
  if (!json::getInt(obj, m_shape.EXIT_CODE, m_exitCode)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.EXIT_CODE);
  }
  // EXIT_CODE_VALID (optional, defaults to true)
  bool exitCodeValid = false;
  if (json::getBool(obj, m_shape.EXIT_CODE_VALID, exitCodeValid)) { m_exitCodeValid = exitCodeValid; }
  // GROUP_NAME
  if (!json::getString(obj, m_shape.GROUP_NAME, m_groupName)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.GROUP_NAME);
  }
  // GROUP_ID
  if (!json::getString(obj, m_shape.GROUP_ID, m_groupId)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.GROUP_ID);
  }
  // resource data (optional; values may be serialized as JSON null)
  bool resourceDataValid = false;
  if (json::getBool(obj, m_shape.RESOURCES_VALID, resourceDataValid)) { m_resourceDataValid = resourceDataValid; }
  std::int64_t uptimeSeconds = 0;
  if (json::getInt64(obj, m_shape.UPTIME_SECONDS, uptimeSeconds)) { m_uptimeSeconds = uptimeSeconds; }
  double cpuPercent = 0.0;
  if (json::getDouble(obj, m_shape.CPU_PERCENT, cpuPercent)) { m_cpuPercent = cpuPercent; }
  std::uint64_t memoryBytes = 0;
  if (json::getUint64(obj, m_shape.MEMORY_BYTES, memoryBytes)) { m_memoryBytes = memoryBytes; }
  return success;
}
}// namespace lintel
