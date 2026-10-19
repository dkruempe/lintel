#ifndef LINTEL_LOGGERCOMPONENT_H
#define LINTEL_LOGGERCOMPONENT_H

#include <lintel/core/configuration/EnvironmentConfiguration.h>
#include <tinyxml2.h>

#include <memory>

#include "lintel/core/configuration/Component.h"
#include "lintel/core/configuration/LoggerSinkConfiguration.h"

namespace lintel {
/** Component for parsing logger configurations from XML */
class LoggerComponent : public Component
{
private:
  /** The environment configuration for resolving environment variables */
  std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;

  /** XML element name constants for logger parsing */
  static const struct Shapes
  {
    const char *const CONFIG_ROOT = "Loggers";
    const char *const LOGGER_ROOT = "Logger";
    const char *const LOGGER_PATTERN = "pattern";
    const char *const LOGGER_LEVEL = "level";
    const char *const LOGGER_ASYNC = "async";
    const char *const LOGGER_PROCESS = "process_name";
    const char *const LOGGER_JSON = "json";
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
  std::shared_ptr<Entry> parseLoggerPath(tinyxml2::XMLElement *loggerElement, int32_t &lineNumber);

  /** Parse a single logger configuration from XML
   * @param loggerElement The XML element to parse
   * @param lineNumber Current line number (in/out)
   * @param lineOffset Line offset for error reporting
   * @return Parsed entry or nullptr */
  std::shared_ptr<Entry> parseLogger(tinyxml2::XMLElement *loggerElement, int32_t &lineNumber, int32_t lineOffset);

  /** Inherited defaults applied to a logger sink that does not override them. */
  struct SinkDefaults
  {
    /** inherited log level */
    const char *level = nullptr;
    /** inherited log pattern */
    const char *pattern = nullptr;
  };

  /** Parse a logger sink configuration from XML
   * @param loggerSinkElement The XML element to parse
   * @param lineNo Current line number
   * @param defaults Inherited log level and pattern
   * @return Parsed logger sink configuration */
  LoggerSinkConfiguration
    parseLoggerSink(tinyxml2::XMLElement *loggerSinkElement, int32_t lineNo, const SinkDefaults &defaults);

public:
  /** Construct a LoggerComponent
   * @param environmentConfiguration The environment configuration */
  LoggerComponent(std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

  /** Parse logger configuration XML
   * @param content XML content to parse
   * @param fileName Source file name for error reporting
   * @param lineOffset Line offset for error reporting
   * @return Vector of parsed logger entries */
  std::vector<std::shared_ptr<Entry>>
    parse(const std::string &content, int32_t lineOffset, const std::string &fileName) override;
};

}// namespace lintel
#endif// LINTEL_LOGGERCOMPONENT_H
