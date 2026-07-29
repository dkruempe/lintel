#ifndef CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
#define CPP_BASE_LIBRARY_LOGGERCOMPONENT_H

#include <base_library/features/base/configuration/EnvironmentConfiguration.h>
#include <tinyxml2.h>

#include <memory>

#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/LoggerSinkConfiguration.h"

class LoggerComponent : public Component {
private:
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;

    static const struct Shapes {
        const char *const CONFIG_ROOT = "Loggers";
        const char *const LOGGER_ROOT = "Logger";
        const char *const LOGGER_PATTERN = "pattern";
        const char *const LOGGER_LEVEL = "level";
        const char *const LOGGER_ASYNC = "async";
        const char *const LOGGER_PROCESS = "process_name";
        const char *const LOGGER_SINK_ROOT = "LoggerSink";
        const char *const LOGGER_SINK_TYPE = "type";
        const char *const LOGGER_SINK_LEVEL = "level";
        const char *const LOGGER_SINK_PATTERN = "pattern";
        const char *const LOGGER_SINK_CONNECTION = "connection";
        const char *const LOGGER_SINK_PORT = "port";
        const char *const LOGGER_SINK_TIME = "time";
        const char *const LOGGER_SINK_FILE_NAME = "file_name";
        const char *const LOGGER_SINK_SIZE = "size";
        const char *const LOGGER_SINK_MAX_FILES = "max_files";
        const char *const LOGGER_SINK_ID = "id";
        const char *const PATH_ROOT = "Path";
        const char *const PATH_PATH = "path";
        const char *const PATH_CREATE_PROCESS_SUB_DIR = "create_sub_dirs";
    } shape;

    std::shared_ptr<Entry> parseLoggerPath(tinyxml2::XMLElement *loggerElement,
                                           int32_t &lineNumber);

    std::shared_ptr<Entry> parseLogger(tinyxml2::XMLElement *loggerElement,
                                       int32_t &lineNumber, int32_t lineOffset);

    LoggerSinkConfiguration parseLoggerSink(
            tinyxml2::XMLElement *loggerSinkElement, int32_t lineNo,
            const char *level, const char *pattern);

public:
    LoggerComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
