#ifndef LINTEL_LOGGERSINKCONFIGURATION_H
#define LINTEL_LOGGERSINKCONFIGURATION_H

#include <string>

/** Configuration for a single logger sink (output destination) */
class LoggerSinkConfiguration {
public:
    /** Supported logger sink types */
    enum LoggerSinkType {
        ConsoleSink, /**< Console output */
        TcpSink, /**< TCP network output */
        DailyFileSink, /**< Daily rotating file */
        RotatingFileSink, /**< Size-based rotating file */
        SysLogSink /**< System log output */
    };

private:
    /** The sink type */
    LoggerSinkType m_type;
    /** The log level for this sink */
    std::string m_level;
    /** The log pattern for this sink */
    std::string m_pattern;
    /** File name (daily/rotating) */
    std::string m_fileName;
    /** Rotation time (daily) */
    std::string m_time;
    /** Maximum file size (rotating) */
    std::size_t m_fileSize;
    /** Maximum number of files (rotating) */
    std::size_t m_maxFiles;
    /** Connection string (TCP) */
    std::string m_connection;
    /** Port number (TCP) */
    int32_t m_port;
    /** Syslog identifier (syslog) */
    std::string m_id;

public:
    /** Construct a console sink configuration
     * @param type The sink type
     * @param level The log level
     * @param pattern The log pattern */
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern);

    /** Construct a daily file sink configuration
     * @param type The sink type
     * @param level The log level
     * @param pattern The log pattern
     * @param fileName The base file name
     * @param time The rotation time */
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string fileName,
                            std::string time);

    /** Construct a rotating file sink configuration
     * @param type The sink type
     * @param level The log level
     * @param pattern The log pattern
     * @param fileName The base file name
     * @param fileSize The maximum file size
     * @param maxFiles The maximum number of files */
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string fileName,
                            std::size_t fileSize, std::size_t maxFiles);

    /** Construct a TCP sink configuration
     * @param type The sink type
     * @param level The log level
     * @param pattern The log pattern
     * @param connection The connection string
     * @param port The port number */
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string connection,
                            int32_t port);

    /** Construct a syslog sink configuration
     * @param type The sink type
     * @param level The log level
     * @param pattern The log pattern
     * @param id The syslog identifier */
    LoggerSinkConfiguration(LoggerSinkType type, std::string level,
                            std::string pattern, std::string id);

    /** Get the sink type
     * @return The sink type */
    [[nodiscard]] LoggerSinkType getType() const;

    /** Get the log level
     * @return The level string */
    [[nodiscard]] const std::string &getLevel() const;

    /** Get the log pattern
     * @return The pattern string */
    [[nodiscard]] const std::string &getPattern() const;

    /** Get the file name
     * @return The file name */
    [[nodiscard]] const std::string &getFileName() const;

    /** Get the rotation time
     * @return The time string */
    [[nodiscard]] const std::string &getTime() const;

    /** Get the maximum file size
     * @return The file size in bytes */
    [[nodiscard]] size_t getFileSize() const;

    /** Get the TCP connection string
     * @return The connection string */
    [[nodiscard]] const std::string &getConnection() const;

    /** Get the TCP port
     * @return The port number */
    [[nodiscard]] int32_t getPort() const;

    /** Get the syslog identifier
     * @return The identifier string */
    [[nodiscard]] const std::string &getId() const;

    /** Get the maximum number of rotated files
     * @return The maximum files count */
    [[nodiscard]] size_t getMaxFiles() const;
};

#endif  // LINTEL_LOGGERSINKCONFIGURATION_H
