#include "base_library/features/base/configuration/LoggerSinkConfiguration.h"

LoggerSinkConfiguration::LoggerSinkConfiguration(
        LoggerSinkConfiguration::LoggerSinkType type, std::string level,
        std::string pattern)
        : m_type(type),
          m_level(std::move(level)),
          m_pattern(std::move(pattern)),
          m_fileSize(0),
          m_maxFiles(0),
          m_port(-1) {}

LoggerSinkConfiguration::LoggerSinkConfiguration(
        LoggerSinkConfiguration::LoggerSinkType type, std::string level,
        std::string pattern, std::string fileName, std::string time)
        : m_type(type),
          m_level(std::move(level)),
          m_pattern(std::move(pattern)),
          m_fileName(std::move(fileName)),
          m_time(std::move(time)),
          m_fileSize(0),
          m_maxFiles(0),
          m_port(-1) {}

LoggerSinkConfiguration::LoggerSinkConfiguration(
        LoggerSinkConfiguration::LoggerSinkType type, std::string level,
        std::string pattern, std::string fileName, std::size_t fileSize,
        std::size_t maxFiles)
        : m_type(type),
          m_level(std::move(level)),
          m_pattern(std::move(pattern)),
          m_fileName(std::move(fileName)),
          m_fileSize(fileSize),
          m_maxFiles(maxFiles),
          m_port(-1) {}

LoggerSinkConfiguration::LoggerSinkConfiguration(
        LoggerSinkConfiguration::LoggerSinkType type, std::string level,
        std::string pattern, std::string connection, int32_t port)
        : m_type(type),
          m_level(std::move(level)),
          m_pattern(std::move(pattern)),
          m_fileSize(0),
          m_maxFiles(0),
          m_connection(std::move(connection)),
          m_port(port) {}

LoggerSinkConfiguration::LoggerSinkConfiguration(
        LoggerSinkConfiguration::LoggerSinkType type, std::string level,
        std::string pattern, std::string id)
        : m_type(type),
          m_level(std::move(level)),
          m_pattern(std::move(pattern)),
          m_fileSize(0),
          m_maxFiles(0),
          m_port(-1),
          m_id(std::move(id)) {}

LoggerSinkConfiguration::LoggerSinkType LoggerSinkConfiguration::getType()
const {
    return m_type;
}

const std::string &LoggerSinkConfiguration::getLevel() const { return m_level; }

const std::string &LoggerSinkConfiguration::getPattern() const {
    return m_pattern;
}

const std::string &LoggerSinkConfiguration::getFileName() const {
    return m_fileName;
}

const std::string &LoggerSinkConfiguration::getTime() const { return m_time; }

size_t LoggerSinkConfiguration::getFileSize() const { return m_fileSize; }

const std::string &LoggerSinkConfiguration::getConnection() const {
    return m_connection;
}

int32_t LoggerSinkConfiguration::getPort() const { return m_port; }

const std::string &LoggerSinkConfiguration::getId() const { return m_id; }

size_t LoggerSinkConfiguration::getMaxFiles() const { return m_maxFiles; }
