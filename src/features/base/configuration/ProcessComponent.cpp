#include "lintel/features/base/configuration/ProcessComponent.h"

#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>

#include "lintel/core/utils/StringUtils.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/ConfigurationException.h"
#include "lintel/features/base/configuration/ProcessEntry.h"
#include "lintel/features/base/models/Process.h"

const ProcessComponent::Shapes ProcessComponent::m_shapes{};

namespace {

std::chrono::milliseconds parseDurationMs(const char *value, const char *attribute)
{
  if (value == nullptr) { return std::chrono::milliseconds(0); }
  try {
    return std::chrono::milliseconds(std::stoll(value));
  } catch (const std::exception &) {
    throw ConfigurationException(std::string(attribute), "invalid millisecond value: " + std::string(value), 0);
  }
}

int parseOptionalInt(const char *value, int fallback)
{
  if (value == nullptr) { return fallback; }
  try {
    return std::stoi(value);
  } catch (const std::exception &) {
    return fallback;
  }
}

}// namespace

ProcessComponent::ProcessComponent() : Component(m_shapes.CONFIG_ROOT) {}

std::shared_ptr<Entry> ProcessComponent::parseProcess(tinyxml2::XMLElement *processElement, int32_t lineOffset)
{
  int32_t lineNumber = lineOffset + processElement->GetLineNum();
  const char *name = processElement->Attribute(m_shapes.PROCESS_NAME);
  const char *autoRestartsStr = processElement->Attribute(m_shapes.PROCESS_AUTO_RESTART);
  const char *maxRestartsStr = processElement->Attribute(m_shapes.PROCESS_MAX_RESTARTS);
  const char *argsStr = processElement->Attribute(m_shapes.PROCESS_ARGS);
  // optional bootstrap config name for the child process
  const char *configName = processElement->Attribute(m_shapes.PROCESS_CONFIG);

  if (name == nullptr) { throw ConfigurationException(getConfigRoot(), "process - name is null", lineNumber); }

  if (autoRestartsStr == nullptr) {
    throw ConfigurationException(getConfigRoot(), "process - autoRestart is null", lineNumber);
  }

  if (maxRestartsStr == nullptr) {
    throw ConfigurationException(getConfigRoot(), "process - maxRestarts is null", lineNumber);
  }

  if (argsStr == nullptr) { throw ConfigurationException(getConfigRoot(), "process - args is null", lineNumber); }
  std::vector<std::string> args;
  if (std::strcmp(argsStr, "") != 0) { args = StringUtils::split(argsStr, ','); }
  std::shared_ptr<Process> process = std::make_shared<Process>(name, args, configName == nullptr ? "" : configName);
  bool isAutoRestart = std::strcmp(autoRestartsStr, "true") == 0;
  int32_t maxRestarts = std::stoi(maxRestartsStr);
  if (isAutoRestart) {
    process->enableAutoStart(maxRestarts);
  } else {
    process->disableAutoStart();
  }
  // automation policy (all optional)
  process->setRestartDelay(
    parseDurationMs(processElement->Attribute(m_shapes.PROCESS_RESTART_DELAY), m_shapes.PROCESS_RESTART_DELAY));
  process->setRestartDelayMax(
    parseDurationMs(processElement->Attribute(m_shapes.PROCESS_RESTART_DELAY_MAX), m_shapes.PROCESS_RESTART_DELAY_MAX));
  process->setMinUptime(
    parseDurationMs(processElement->Attribute(m_shapes.PROCESS_MIN_UPTIME), m_shapes.PROCESS_MIN_UPTIME));
  process->setRestartWindow(
    parseDurationMs(processElement->Attribute(m_shapes.PROCESS_RESTART_WINDOW), m_shapes.PROCESS_RESTART_WINDOW));
  process->setRestartInterval(
    parseDurationMs(processElement->Attribute(m_shapes.PROCESS_RESTART_INTERVAL), m_shapes.PROCESS_RESTART_INTERVAL));
  process->setMaxRestartRate(parseOptionalInt(processElement->Attribute(m_shapes.PROCESS_MAX_RESTART_RATE), -1));

  const char *activeFrom = processElement->Attribute(m_shapes.PROCESS_ACTIVE_FROM);
  const char *activeTo = processElement->Attribute(m_shapes.PROCESS_ACTIVE_TO);
  if (activeFrom != nullptr || activeTo != nullptr) {
    process->setActiveFromHour(
      activeFrom == nullptr ? std::nullopt : std::optional<int32_t>(parseOptionalInt(activeFrom, 0)));
    process->setActiveToHour(
      activeTo == nullptr ? std::nullopt : std::optional<int32_t>(parseOptionalInt(activeTo, 0)));
  }

  const char *cpuNotify = processElement->Attribute(m_shapes.PROCESS_CPU_NOTIFY);
  if (cpuNotify != nullptr) {
    try {
      process->setCpuNotify(std::optional<double>(std::stod(cpuNotify)));
    } catch (const std::exception &) {
      process->setCpuNotify(std::nullopt);
    }
  }
  const char *memoryNotify = processElement->Attribute(m_shapes.PROCESS_MEMORY_NOTIFY);
  if (memoryNotify != nullptr) {
    try {
      process->setMemoryNotify(std::optional<uint64_t>(std::stoull(memoryNotify)));
    } catch (const std::exception &) {
      process->setMemoryNotify(std::nullopt);
    }
  }
  return std::make_shared<ProcessEntry>(type_name<ProcessComponent>(), process);
}

std::vector<std::shared_ptr<Entry>>
  ProcessComponent::parse(const std::string &content, const std::string &fileName, int32_t lineOffset)
{
  std::vector<std::shared_ptr<Entry>> processEntries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());
  tinyxml2::XMLElement *rootNode = document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) { return processEntries; }
  for (tinyxml2::XMLElement *processElement = rootNode->FirstChildElement(); processElement != nullptr;
    processElement = processElement->NextSiblingElement()) {
    if (std::strcmp(processElement->Name(), m_shapes.PROCESS_ROOT) == 0) {
      processEntries.push_back(parseProcess(processElement, lineOffset));
    } else if (std::strcmp(processElement->Name(), m_shapes.PROCESS_GROUP_ROOT) == 0) {
      processEntries.push_back(parseProcessGroup(processElement, lineOffset));
    }
  }
  return processEntries;
}

std::shared_ptr<Entry> ProcessComponent::parseProcessGroup(tinyxml2::XMLElement *processGroupElement,
  int32_t lineOffset)
{
  std::vector<Process> entries;
  for (tinyxml2::XMLElement *processElement = processGroupElement->FirstChildElement(); processElement != nullptr;
    processElement = processElement->NextSiblingElement()) {
    if (std::strcmp(processElement->Name(), m_shapes.PROCESS_ROOT) != 0) { continue; }
    std::shared_ptr<ProcessEntry> processEntry =
      std::static_pointer_cast<ProcessEntry>(parseProcess(processElement, lineOffset));
    entries.push_back(*processEntry->getProcess());
  }
  const char *groupName = processGroupElement->Attribute(m_shapes.PROCESS_GROUP_NAME);
  if (groupName == nullptr) {
    throw ConfigurationException(
      m_shapes.CONFIG_ROOT, "process-group - name not found", lineOffset + processGroupElement->GetLineNum());
  }
  std::shared_ptr<ProcessGroup> processGroup = std::make_shared<ProcessGroup>(groupName, entries);
  return std::make_shared<ProcessEntry>(type_name<ProcessComponent>(), processGroup);
}
