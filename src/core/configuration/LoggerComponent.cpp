#include "base_library/core/configuration/LoggerComponent.h"

#include <memory>

#include "base_library/core/configuration/ConfigurationException.h"
#include "base_library/core/configuration/LoggerEnrty.h"
#include "base_library/core/configuration/LoggerPathConfiguration.h"
import base_library.core.utils;
import base_library.core.utils.type_name;

const LoggerComponent::Shapes LoggerComponent::shape{};

LoggerComponent::LoggerComponent(
        std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
        : Component(shape.CONFIG_ROOT),
          m_environmentConfiguration(std::move(environmentConfiguration)) {}

std::vector<std::shared_ptr<Entry>> LoggerComponent::parse(
        const std::string &content, const std::string &fileName,
        int32_t lineOffset) {
    std::vector<std::shared_ptr<Entry>> loggerEntries;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());
    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return loggerEntries;
    }
    for (tinyxml2::XMLElement *loggerElement = rootNode->FirstChildElement();
         loggerElement != nullptr;
         loggerElement = loggerElement->NextSiblingElement()) {
        if (std::strcmp(loggerElement->Name(), shape.LOGGER_ROOT) != 0 &&
            std::strcmp(loggerElement->Name(), shape.PATH_ROOT) != 0) {
            continue;
        }
        bool isLoggerElement =
                std::strcmp(loggerElement->Name(), shape.LOGGER_ROOT) == 0;
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
        tinyxml2::XMLElement *loggerElement, int32_t &lineNumber,
        int32_t lineOffset) {
    // get attributes of <Logger>
    const char *processName =
            loggerElement->Attribute(shape.LOGGER_PROCESS);
    const char *pattern = loggerElement->Attribute(shape.LOGGER_PATTERN);
    const char *level = loggerElement->Attribute(shape.LOGGER_LEVEL);
    const char *async = loggerElement->Attribute(shape.LOGGER_ASYNC);
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
    for (tinyxml2::XMLElement *loggerSinkElement =
            loggerElement->FirstChildElement();
         loggerSinkElement != nullptr;
         loggerSinkElement = loggerSinkElement->NextSiblingElement()) {
        int32_t lineNo = lineOffset + loggerSinkElement->GetLineNum();
        if (std::strcmp(loggerSinkElement->Name(),
                        shape.LOGGER_SINK_ROOT) != 0) {
            continue;
        }
        loggerSinks.push_back(
                parseLoggerSink(loggerSinkElement, lineNo, level, pattern));
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
        tinyxml2::XMLElement *loggerElement, int32_t &lineNumber) {
    const char *pathStr = loggerElement->Attribute(shape.PATH_PATH);
    const char *createSubDirectoriesStr =
            loggerElement->Attribute(shape.PATH_CREATE_PROCESS_SUB_DIR);
    if (pathStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "path is nullptr",
                                     lineNumber);
    }
    if (createSubDirectoriesStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "create_sub_dirs nullptr",
                                     lineNumber);
    }
    std::string tmpPath = pathStr;
    if (!tmpPath.empty() && tmpPath[0] == '~') {
        std::string restPath(tmpPath.begin() + 1, tmpPath.end());
        tmpPath = m_environmentConfiguration->of(EnvironmentConfiguration::Home);
        tmpPath += '/';
        tmpPath += restPath;
    }
    std::filesystem::path path(tmpPath);
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

namespace {
LoggerSinkConfiguration::LoggerSinkType parseSinkType(
        const char *typeStr, int32_t lineNo, const std::string &configRoot) {
    if (std::strcmp(typeStr, "ConsoleSink") == 0) { return LoggerSinkConfiguration::ConsoleSink; }
    if (std::strcmp(typeStr, "TcpSink") == 0) { return LoggerSinkConfiguration::TcpSink; }
    if (std::strcmp(typeStr, "DailyFileSink") == 0) { return LoggerSinkConfiguration::DailyFileSink; }
    if (std::strcmp(typeStr, "RotatingFileSink") == 0) { return LoggerSinkConfiguration::RotatingFileSink; }
    if (std::strcmp(typeStr, "SysLogSink") == 0) { return LoggerSinkConfiguration::SysLogSink; }
    throw ConfigurationException(configRoot, "logger sink unknown type", lineNo);
}
}

LoggerSinkConfiguration LoggerComponent::parseLoggerSink(
        tinyxml2::XMLElement *loggerSinkElement, int32_t lineNo,
        const char *level, const char *pattern) {
    const char *typeStr =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_TYPE);
    if (typeStr == nullptr) {
        throw ConfigurationException(getConfigRoot(), "logger sink type is null",
                                     lineNo);
    }
    auto type = parseSinkType(typeStr, lineNo, getConfigRoot());
    const char *levelSink =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_LEVEL);
    const char *patternSink =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_PATTERN);
    const char *fileName =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_FILE_NAME);
    const char *time =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_TIME);
    const char *size =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_SIZE);
    const char *maxFiles =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_MAX_FILES);
    const char *connection =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_CONNECTION);
    const char *port =
            loggerSinkElement->Attribute(shape.LOGGER_SINK_PORT);
    const char *id = loggerSinkElement->Attribute(shape.LOGGER_SINK_ID);
    std::string loggerSinkLevel = levelSink != nullptr ? levelSink : level;
    std::string loggerSinkPattern =
            patternSink != nullptr ? patternSink : pattern;
    switch (type) {
        case LoggerSinkConfiguration::ConsoleSink: {
            return {type, loggerSinkLevel, loggerSinkPattern};
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
            return {type, loggerSinkLevel, loggerSinkPattern,
                    connection, std::stoi(port)};
        }
        case LoggerSinkConfiguration::DailyFileSink: {
            if (time == nullptr) {
                throw ConfigurationException(getConfigRoot(),
                                             "LoggerSink time not defined", lineNo);
            }
            std::string loggerSinkFileName = fileName == nullptr ? "" : fileName;
            return {type, loggerSinkLevel, loggerSinkPattern,
                    loggerSinkFileName, time};
        }
        case LoggerSinkConfiguration::RotatingFileSink: {
            std::string loggerSinkFileName = fileName == nullptr ? "" : fileName;
            if (size == nullptr) {
                throw ConfigurationException(getConfigRoot(),
                                             "LoggerSinks size nullptr", lineNo);
            }
            if (maxFiles == nullptr) {
                throw ConfigurationException(getConfigRoot(),
                                             "LoggerSinks maxFiles nullptr", lineNo);
            }
            return {type, loggerSinkLevel, loggerSinkPattern,
                    loggerSinkFileName, convertToBytes(size),
                    std::stoul(maxFiles)};
        }
        case LoggerSinkConfiguration::SysLogSink: {
            if (id == nullptr) {
                throw ConfigurationException(getConfigRoot(),
                                             "LoggerSink id == nullptr", lineNo);
            }
            return {type, loggerSinkLevel, loggerSinkPattern, id};
        }
    }
    throw ConfigurationException(getConfigRoot(), "logger sink unknown type",
                                 lineNo);
}
