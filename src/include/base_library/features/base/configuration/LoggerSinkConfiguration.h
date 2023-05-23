#ifndef CPP_BASE_LIBRARY_LOGGERSINKCONFIGURATION_H
#define CPP_BASE_LIBRARY_LOGGERSINKCONFIGURATION_H

#include <string>

class LoggerSinkConfiguration {
public:
    enum LoggerSinkType {
        ConsoleSink,
        TcpSink,
        DailyFileSink,
        RotatingFileSink,
        SysLogSink
    };

private:
    // GENERAL
    LoggerSinkType m_type;
    std::string m_level;
    std::string m_pattern;
    // DAILY / ROTATING
    std::string m_fileName;
    // DAILY
    std::string m_time;
    // ROTATING
    std::size_t m_fileSize;
    std::size_t m_maxFiles;
    // TCP
    std::string m_connection;
    int32_t m_port;
    // SYSLOG
    std::string m_id;

public:
    // CONSOLE
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern);

    // DAILY
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string fileName,
                            std::string time);

    // ROTATING
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string fileName,
                            std::size_t fileSize, std::size_t maxFiles);

    // TCP
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string connection,
                            int32_t port);

    // SYSLOG
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string id);

    [[nodiscard]] LoggerSinkType getType() const;

    [[nodiscard]] const std::string &getLevel() const;

    [[nodiscard]] const std::string &getPattern() const;

    [[nodiscard]] const std::string &getFileName() const;

    [[nodiscard]] const std::string &getTime() const;

    [[nodiscard]] size_t getFileSize() const;

    [[nodiscard]] const std::string &getConnection() const;

    [[nodiscard]] int32_t getPort() const;

    [[nodiscard]] const std::string &getId() const;

    [[nodiscard]] size_t getMaxFiles() const;
};

#endif  // CPP_BASE_LIBRARY_LOGGERSINKCONFIGURATION_H
