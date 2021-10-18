#ifndef CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
#define CPP_BASE_LIBRARY_LOGGERCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"

class LoggerComponent : public Component {
 private:
  static struct Shapes {
    const std::string CONFIG_ROOT = "Loggers";
    // LOGGER
    const std::string LOGGER_ROOT = "Logger";
    const std::string LOGGER_PATTERN = "pattern";
    const std::string LOGGER_LEVEL = "level";
    const std::string LOGGER_ASYNC = "async";
    const std::string LOGGER_PROCESS = "process_name";
    const std::string LOGGER_SINK_ROOT = "LoggerSink";
    const std::string LOGGER_SINK_TYPE = "type";
    const std::string LOGGER_SINK_LEVEL = "level";
    const std::string LOGGER_SINK_PATTERN = "pattern";
    const std::string LOGGER_SINK_CONNECTION = "connection";
    const std::string LOGGER_SINK_PORT = "port";
    const std::string LOGGER_SINK_TIME = "time";
    const std::string LOGGER_SINK_FILE_NAME = "file_name";
    const std::string LOGGER_SINK_SIZE = "size";
    const std::string LOGGER_SINK_MAX_FILES = "max_files";
    const std::string LOGGER_SINK_ID = "id";
    // PATH
    const std::string PATH_ROOT = "Path";
    const std::string PATH_PATH = "path";
    const std::string PATH_CREATE_PROCESS_SUB_DIR = "create_sub_dirs";

  } shape;

  std::shared_ptr<Entry> parseLoggerPath(tinyxml2::XMLElement* loggerElement, int32_t &lineNumber);

  std::shared_ptr<Entry> parseLogger(tinyxml2::XMLElement* loggerElement, int32_t &lineNumber, int32_t lineOffset);

 public:
  LoggerComponent();

  std::vector<std::shared_ptr<Entry>> parse(const std::string& content,
                                            const std::string& fileName,
                                            int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_LOGGERCOMPONENT_H
