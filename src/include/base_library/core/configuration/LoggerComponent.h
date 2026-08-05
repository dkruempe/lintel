#ifndef CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
#define CPP_BASE_LIBRARY_LOGGERCOMPONENT_H

#include <base_library/core/configuration/EnvironmentConfiguration.h>
#include <tinyxml2.h>

#include <memory>

#include "base_library/core/configuration/Component.h"
#include "base_library/core/configuration/LoggerSinkConfiguration.h"

/** Component for parsing logger configurations from XML */
class LoggerComponent : public Component {
private:
    /** The environment configuration for resolving environment variables */
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;

    /** XML element name constants for logger parsing */
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

    /** Parse a logger path configuration from XML
     * @param loggerElement The XML element to parse
     * @param lineNumber Current line number (in/out)
     * @return Parsed entry or nullptr */
    std::shared_ptr<Entry> parseLoggerPath(tinyxml2::XMLElement *loggerElement,
                                           int32_t &lineNumber);

    /** Parse a single logger configuration from XML
     * @param loggerElement The XML element to parse
     * @param lineNumber Current line number (in/out)
     * @param lineOffset Line offset for error reporting
     * @return Parsed entry or nullptr */
    std::shared_ptr<Entry> parseLogger(tinyxml2::XMLElement *loggerElement,
                                       int32_t &lineNumber, int32_t lineOffset);

    /** Parse a logger sink configuration from XML
     * @param loggerSinkElement The XML element to parse
     * @param lineNo Current line number
     * @param level Inherited log level
     * @param pattern Inherited log pattern
     * @return Parsed logger sink configuration */
    LoggerSinkConfiguration parseLoggerSink(
            tinyxml2::XMLElement *loggerSinkElement, int32_t lineNo,
            const char *level, const char *pattern);

public:
    /** Construct a LoggerComponent
     * @param environmentConfiguration The environment configuration */
    LoggerComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    /** Parse logger configuration XML
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed logger entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
