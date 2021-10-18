#include "base_library/features/base/configuration/LoggerComponent.h"

#include "base_library/core/utils/TypeName.h"
#include "base_library/features/base/configuration/ConfigurationException.h"
#include "base_library/features/base/configuration/LoggerEnrty.h"
#include "base_library/features/base/configuration/LoggerPathConfiguration.h"

LoggerComponent::Shapes LoggerComponent::shape{};

LoggerComponent::LoggerComponent() : Component(shape.CONFIG_ROOT) {}
std::vector<std::shared_ptr<Entry>> LoggerComponent::parse(
    const std::string& content, const std::string& fileName,
    int32_t lineOffset) {
  std::vector<std::shared_ptr<Entry>> loggerEntries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());
  tinyxml2::XMLElement* rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return loggerEntries;
  }
  for (tinyxml2::XMLElement* loggerElement = rootNode->FirstChildElement();
       loggerElement != nullptr;
       loggerElement = loggerElement->NextSiblingElement()) {
    if (std::strcmp(loggerElement->Name(), shape.LOGGER_ROOT.c_str()) != 0 &&
        std::strcmp(loggerElement->Name(), shape.PATH_ROOT.c_str()) != 0) {
      continue;
    }
    bool isLoggerElement =
        std::strcmp(loggerElement->Name(), shape.LOGGER_ROOT.c_str()) == 0;
    int lineNumber = lineOffset + loggerElement->GetLineNum();
    if (isLoggerElement) {
      // parse logger element + sub child's LoggerSink elements
      loggerEntries.push_back(
          parseLogger(loggerElement, lineNumber, lineOffset));
      continue;
    }
    // parse logger path element
    loggerEntries.push_back(parseLoggerPath(loggerElement, lineNumber));
  }
  return loggerEntries;
}
std::shared_ptr<Entry> LoggerComponent::parseLogger(
    tinyxml2::XMLElement* loggerElement, int32_t& lineNumber,
    int32_t lineOffset) {
  // get attributes of <Logger>
  const char* processName =
      loggerElement->Attribute(shape.LOGGER_PROCESS.c_str());
  const char* pattern = loggerElement->Attribute(shape.LOGGER_PATTERN.c_str());
  const char* level = loggerElement->Attribute(shape.LOGGER_LEVEL.c_str());
  const char* async = loggerElement->Attribute(shape.LOGGER_ASYNC.c_str());
  // validation of logger attributes
  if (processName == nullptr) {
    throw ConfigurationException(getConfigRoot(), "logger process_name is null",
                                 lineNumber);
  }
  if (pattern == nullptr) {
    pattern = "";
  }
  if (level == nullptr) {
    throw ConfigurationException(getConfigRoot(), "logger level is null",
                                 lineNumber);
  }
  if (async == nullptr) {
    throw ConfigurationException(getConfigRoot(), "logger async is null",
                                 lineNumber);
  }
  // iterate over childs of loggerEntry
  std::vector<LoggerSinkConfiguration> loggerSinks;
  for (tinyxml2::XMLElement* loggerSinkElement =
           loggerElement->FirstChildElement();
       loggerSinkElement != nullptr;
       loggerSinkElement = loggerSinkElement->NextSiblingElement()) {
    int32_t lineNo = lineOffset + loggerSinkElement->GetLineNum();
    if (std::strcmp(loggerSinkElement->Name(),
                    shape.LOGGER_SINK_ROOT.c_str()) != 0) {
      continue;
    }
    const char* typeStr =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_TYPE.c_str());
    if (typeStr == nullptr) {
      throw ConfigurationException(getConfigRoot(), "logger sink type is null",
                                   lineNo);
    }
    LoggerSinkConfiguration::LoggerSinkType type;
    if (std::strcmp(typeStr, "ConsoleSink") == 0) {
      type = LoggerSinkConfiguration::ConsoleSink;
    } else if (std::strcmp(typeStr, "TcpSink") == 0) {
      type = LoggerSinkConfiguration::TcpSink;
    } else if (std::strcmp(typeStr, "DailyFileSink") == 0) {
      type = LoggerSinkConfiguration::DailyFileSink;
    } else if (std::strcmp(typeStr, "RotatingFileSink") == 0) {
      type = LoggerSinkConfiguration::RotatingFileSink;
    } else if (std::strcmp(typeStr, "SysLogSink") == 0) {
      type = LoggerSinkConfiguration::SysLogSink;
    } else {
      throw ConfigurationException(getConfigRoot(), "logger sink unknown type",
                                   lineNo);
    }
    // optional
    const char* levelSink =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_LEVEL.c_str());
    const char* patternSink =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_PATTERN.c_str());
    // depends on logger sink type
    // DailyFileSink, RotatingFileSink
    const char* fileName =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_FILE_NAME.c_str());
    // DailyFileSink
    const char* time =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_TIME.c_str());
    // RotatingFileSink
    const char* size =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_SIZE.c_str());
    const char* maxFiles =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_MAX_FILES.c_str());
    // TcpSink
    const char* connection =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_CONNECTION.c_str());
    const char* port =
        loggerSinkElement->Attribute(shape.LOGGER_SINK_PORT.c_str());
    // SysLogSink
    const char* id = loggerSinkElement->Attribute(shape.LOGGER_SINK_ID.c_str());
    // level / pattern
    std::string loggerSinkLevel = levelSink != nullptr ? levelSink : level;
    std::string loggerSinkPattern =
        patternSink != nullptr ? patternSink : pattern;
    switch (type) {
      case LoggerSinkConfiguration::ConsoleSink: {
        loggerSinks.emplace_back(type, loggerSinkLevel, loggerSinkPattern);
        break;
      }
      case LoggerSinkConfiguration::TcpSink: {
        if (connection == nullptr) {
          throw ConfigurationException(
              getConfigRoot(), "LoggerSink connection == nullptr", lineNo);
        }
        if (port == nullptr) {
          throw ConfigurationException(getConfigRoot(),
                                       "LoggerSink port == nullptr", lineNo);
        }
        loggerSinks.emplace_back(type, loggerSinkLevel, loggerSinkPattern,
                                 connection, std::stoi(port));
        break;
      }
      case LoggerSinkConfiguration::DailyFileSink: {
        if (time == nullptr) {
          throw ConfigurationException(getConfigRoot(),
                                       "LoggerSink time not defined", lineNo);
        }
        std::string loggerSinkFileName = fileName == nullptr ? "" : fileName;
        loggerSinks.emplace_back(type, loggerSinkLevel, loggerSinkPattern,
                                 loggerSinkFileName, time);
        break;
      }
      case LoggerSinkConfiguration::RotatingFileSink: {
        std::string loggerSinkFileName = fileName == nullptr ? "" : fileName;
        if (size == nullptr) {
          throw ConfigurationException(getConfigRoot(),
                                       "LoggerSinks size nullptr", lineNo);
        }
        if (maxFiles == nullptr) {
          throw ConfigurationException(getConfigRoot(), "LoggerSinks maxFiles nullptr", lineNo);
        }
        loggerSinks.emplace_back(type, loggerSinkLevel, loggerSinkPattern,
                                 loggerSinkFileName, std::stoul(size), std::stoul(maxFiles));
        break;
      }
      case LoggerSinkConfiguration::SysLogSink:
        if (id == nullptr) {
          throw ConfigurationException(getConfigRoot(),
                                       "LoggerSink id == nullptr", lineNo);
        }
        loggerSinks.emplace_back(type, loggerSinkLevel, loggerSinkPattern, id);
        break;
    }
  }
  if (loggerSinks.empty()) {
    throw ConfigurationException(getConfigRoot(), "logger sinks not defined",
                                 lineNumber);
  }
  std::shared_ptr<LoggerConfiguration> loggerConfiguration =
      std::make_shared<LoggerConfiguration>(processName, pattern, level, async,
                                            loggerSinks);
  auto loggerEntry = std::make_shared<LoggerEntry>(type_name<LoggerComponent>(),
                                                   loggerConfiguration);
  return loggerEntry;
}
std::shared_ptr<Entry> LoggerComponent::parseLoggerPath(
    tinyxml2::XMLElement* loggerElement, int32_t& lineNumber) {
  const char* pathStr = loggerElement->Attribute(shape.PATH_PATH.c_str());
  const char* createSubDirectoriesStr =
      loggerElement->Attribute(shape.PATH_CREATE_PROCESS_SUB_DIR.c_str());
  if (pathStr == nullptr) {
    throw ConfigurationException(getConfigRoot(), "path is nullptr",
                                 lineNumber);
  }
  if (createSubDirectoriesStr == nullptr) {
    throw ConfigurationException(getConfigRoot(), "create_sub_dirs nullptr",
                                 lineNumber);
  }
  std::filesystem::path path(pathStr);
  if (path.empty() ||
      (!std::filesystem::is_directory(path) && std::filesystem::exists(path))) {
    throw ConfigurationException(
        getConfigRoot(), "path is empty or is not a directory", lineNumber);
  }
  if (std::strcmp(createSubDirectoriesStr, "true") != 0 &&
      std::strcmp(createSubDirectoriesStr, "false") != 0) {
    throw ConfigurationException(
        getConfigRoot(), "value of create_sub_dirs is wrong", lineNumber);
  }
  bool isCreateSubDirectories =
      std::strcmp(createSubDirectoriesStr, "true") == 0;
  auto loggerConfig =
      std::make_shared<LoggerPathConfiguration>(path, isCreateSubDirectories);
  auto loggerEntry =
      std::make_shared<LoggerEntry>(type_name<LoggerComponent>(), loggerConfig);
  return std::static_pointer_cast<Entry>(loggerEntry);
}
