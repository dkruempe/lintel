#include "base_library/features/base/configuration/ProcessComponent.h"

#include <cstring>

#include "base_library/core/utils/StringUtils.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/ConfigurationException.h"
#include "base_library/features/base/configuration/ProcessEntry.h"
#include "base_library/features/base/models/Process.h"

ProcessComponent::Shapes ProcessComponent::m_shapes{};

ProcessComponent::ProcessComponent() : Component(m_shapes.CONFIG_ROOT) {}

std::shared_ptr<Entry> ProcessComponent::parseProcess(
    tinyxml2::XMLElement* processElement, int32_t lineOffset) {
  int32_t lineNumber = lineOffset + processElement->GetLineNum();
  const char* name = processElement->Attribute(m_shapes.PROCESS_NAME.c_str());
  const char* autoRestartsStr =
      processElement->Attribute(m_shapes.PROCESS_AUTO_RESTART.c_str());
  const char* maxRestartsStr =
      processElement->Attribute(m_shapes.PROCESS_MAX_RESTARTS.c_str());
  const char* argsStr =
      processElement->Attribute(m_shapes.PROCESS_ARGS.c_str());

  if (name == nullptr) {
    throw ConfigurationException(getConfigRoot(), "process - name is null",
                                 lineNumber);
  }

  if (autoRestartsStr == nullptr) {
    throw ConfigurationException(getConfigRoot(),
                                 "process - autoRestart is null", lineNumber);
  }

  if (maxRestartsStr == nullptr) {
    throw ConfigurationException(getConfigRoot(),
                                 "process - maxRestarts is null", lineNumber);
  }

  if (argsStr == nullptr) {
    throw ConfigurationException(getConfigRoot(), "process - args is null",
                                 lineNumber);
  }
  std::vector<std::string> args;
  if (std::strcmp(argsStr, "") != 0) {
    args = StringUtils::split(argsStr, ',');
  }
  std::shared_ptr<Process> process = std::make_shared<Process>(name, args);
  bool isAutoRestart = std::strcmp(autoRestartsStr, "true") == 0;
  int32_t maxRestarts = std::stoi(maxRestartsStr);
  if (isAutoRestart) {
    process->enableAutoStart(maxRestarts);
  } else {
    process->disableAutoStart();
  }
  return std::make_shared<ProcessEntry>(type_name<ProcessComponent>(), process);
}

std::vector<std::shared_ptr<Entry>> ProcessComponent::parse(
    const std::string& content, const std::string& fileName,
    int32_t lineOffset) {
  std::vector<std::shared_ptr<Entry>> processEntries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());
  tinyxml2::XMLElement* rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return processEntries;
  }
  for (tinyxml2::XMLElement* processElement = rootNode->FirstChildElement();
       processElement != nullptr;
       processElement = processElement->NextSiblingElement()) {
    if (std::strcmp(processElement->Name(), m_shapes.PROCESS_ROOT.c_str()) ==
        0) {
      processEntries.push_back(parseProcess(processElement, lineOffset));
      continue;
    }
    if (std::strcmp(processElement->Name(),
                    m_shapes.PROCESS_GROUP_ROOT.c_str()) == 0) {
      processEntries.push_back(parseProcessGroup(processElement, lineOffset));

      continue;
    }
  }
  return processEntries;
}
std::shared_ptr<Entry> ProcessComponent::parseProcessGroup(
    tinyxml2::XMLElement* processGroupElement, int32_t lineOffset) {
  std::vector<Process> entries;
  for (tinyxml2::XMLElement* processElement =
           processGroupElement->FirstChildElement();
       processElement != nullptr;
       processElement = processElement->NextSiblingElement()) {
    if (std::strcmp(processElement->Name(), m_shapes.PROCESS_ROOT.c_str()) !=
        0) {
      continue;
    }
    std::shared_ptr<ProcessEntry> processEntry =
        std::static_pointer_cast<ProcessEntry>(
            parseProcess(processElement, lineOffset));
    entries.push_back(*processEntry->getProcess());
  }
  const char* groupName =
      processGroupElement->Attribute(m_shapes.PROCESS_GROUP_NAME.c_str());
  if (groupName == nullptr) {
    throw ConfigurationException(
        m_shapes.CONFIG_ROOT, "process-group - name not found",
        lineOffset + processGroupElement->GetLineNum());
  }
  std::shared_ptr<ProcessGroup> processGroup =
      std::make_shared<ProcessGroup>(groupName, entries);
  return std::make_shared<ProcessEntry>(type_name<ProcessComponent>(),
                                        processGroup);
}
