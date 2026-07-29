#include "base_library/core/services/LoggerService.h"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/tcp_sink.h>

#include <algorithm>
#include <csignal>
#include <iostream>
#include <regex>

#include "base_library/config.h"
#include "base_library/core/exceptions/LoggerServiceNotInitialized.h"
#include "base_library/core/services/DirectoryService.h"
#include "base_library/core/services/FileService.h"
#include "base_library/features/base/configuration/LoggerComponent.h"
#include "base_library/features/base/configuration/LoggerEnrty.h"

// initialization of static variables
std::unique_ptr<LoggerService> LoggerService::m_instance = nullptr;
std::once_flag LoggerService::m_initInstanceFlag;

LoggerService::LoggerService()
        : m_processName(nullptr),
          m_configuration(nullptr),
          m_logger(spdlog::stdout_color_mt("test")) {}

LoggerService::LoggerService(std::shared_ptr<ProcessName> processName,
                             std::shared_ptr<Configuration> configuration)
        : m_processName(std::move(processName)),
          m_configuration(std::move(configuration)),
          m_logger(init()) {}

std::shared_ptr<spdlog::logger> LoggerService::init() {
    std::vector<spdlog::sink_ptr> sinks;
    auto entries = m_configuration->configurationOf<LoggerComponent>();
    auto loggerPath =
            std::find_if(entries.begin(), entries.end(), [](auto entry) {
                std::shared_ptr<LoggerEntry> loggerEntry =
                        std::static_pointer_cast<LoggerEntry>(entry);
                return loggerEntry->isLoggerPathConfiguration();
            });
    if (loggerPath == entries.end()) {
        throw std::runtime_error("Please configure logger path");
    }
    auto loggerConfiguration =
            std::find_if(entries.begin(), entries.end(), [&](auto entry) {
                std::shared_ptr<LoggerEntry> loggerEntry =
                        std::static_pointer_cast<LoggerEntry>(entry);
                std::shared_ptr<LoggerConfiguration> loggerCfg =
                        loggerEntry->getLoggerConfiguration();
                if (loggerCfg == nullptr) {
                    return false;
                }
                std::regex regex(loggerCfg->getProcessName());
                return loggerCfg->getProcessName() ==
                       m_processName->getProcessName() ||
                       std::regex_match(m_processName->getProcessName(), regex);
            });
    if (loggerConfiguration == entries.end()) {
        throw std::runtime_error("No matching Logger Configuration found");
    }
    auto loggerPathEntry = std::static_pointer_cast<LoggerEntry>(*loggerPath);
    auto loggerConfigEntry =
            std::static_pointer_cast<LoggerEntry>(*loggerConfiguration);
    std::shared_ptr<LoggerConfiguration> logConfig =
            loggerConfigEntry->getLoggerConfiguration();
    std::function<void(const LoggerSinkConfiguration &, spdlog::sink_ptr &)>
            setLevelPattern =
            [&](const LoggerSinkConfiguration &loggerSinkConfiguration,
                spdlog::sink_ptr &sinkPtr) {
                // log level
                const std::string &level = loggerSinkConfiguration.getLevel();
                if (level == "critical") {
                    sinkPtr->set_level(spdlog::level::critical);
                } else if (level == "warn") {
                    sinkPtr->set_level(spdlog::level::warn);
                } else if (level == "err") {
                    sinkPtr->set_level(spdlog::level::err);
                } else if (level == "info") {
                    sinkPtr->set_level(spdlog::level::info);
                } else if (level == "debug") {
                    sinkPtr->set_level(spdlog::level::debug);
                } else if (level == "trace") {
                    sinkPtr->set_level(spdlog::level::trace);
                } else if (level == "off") {
                    sinkPtr->set_level(spdlog::level::off);
                }
                const std::string &pattern = loggerSinkConfiguration.getPattern();
                if (!pattern.empty()) {
                    // pattern
                    sinkPtr->set_pattern(loggerSinkConfiguration.getPattern());
                }
            };
    for (const auto &logSinkConfig: logConfig->getLoggerSinks()) {
        switch (logSinkConfig.getType()) {
            case LoggerSinkConfiguration::SysLogSink: {
                break;
            }
            case LoggerSinkConfiguration::ConsoleSink: {
                std::shared_ptr<spdlog::sinks::sink> consoleSink =
                        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                setLevelPattern(logSinkConfig, consoleSink);
                sinks.push_back(consoleSink);
                break;
            }
            case LoggerSinkConfiguration::RotatingFileSink: {
                auto path = loggerPathEntry->getLoggerPathConfiguration()->getPath();
                if (is_regular_file(path)) {
                    throw std::runtime_error("path is file and not directory");
                }
                bool isCreateSubDirectories =
                        loggerPathEntry->getLoggerPathConfiguration()
                                ->isCreateSubDirectories();
                if (isCreateSubDirectories) {
                    path = path.concat(std::filesystem::path::preferred_separator +
                                       m_processName->getProcessName());
                }
                if (!exists(path)) {
                    create_directories(path);
                }
                std::filesystem::path filePath;
                if (logSinkConfig.getFileName().empty()) {
                    filePath = path.concat(std::filesystem::path::preferred_separator +
                                           m_processName->getProcessName() + ".log");
                } else {
                    filePath = path.concat(std::filesystem::path::preferred_separator +
                                           logSinkConfig.getFileName() + ".log");
                }
                std::shared_ptr<spdlog::sinks::sink> rotatingFileSink =
                        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                                filePath, logSinkConfig.getFileSize(),
                                logSinkConfig.getMaxFiles());
                setLevelPattern(logSinkConfig, rotatingFileSink);
                sinks.push_back(rotatingFileSink);
                break;
            }
            case LoggerSinkConfiguration::DailyFileSink: {
                const std::string &time = logSinkConfig.getTime();
                if (time.length() != 5) {
                    throw std::runtime_error("wrong configuration of time");
                }
                int32_t hour = std::stoi(time.substr(0, 2));
                int32_t minute = std::stoi(time.substr(3, 2));
                auto path = loggerPathEntry->getLoggerPathConfiguration()->getPath();
                bool isCreateSubDirectories =
                        loggerPathEntry->getLoggerPathConfiguration()
                                ->isCreateSubDirectories();
                if (isCreateSubDirectories) {
                    path = path.concat(std::filesystem::path::preferred_separator +
                                       m_processName->getProcessName());
                }
                if (is_regular_file(path)) {
                    throw std::runtime_error("path is file and not directory");
                }
                if (!exists(path)) {
                    create_directories(path);
                }
                std::filesystem::path filePath;
                if (logSinkConfig.getFileName().empty()) {
                    filePath = path.concat(std::filesystem::path::preferred_separator +
                                           m_processName->getProcessName() + ".log");
                } else {
                    filePath = path.concat(std::filesystem::path::preferred_separator +
                                           logSinkConfig.getFileName() + ".log");
                }
                std::shared_ptr<spdlog::sinks::sink> dailyFileSink =
                        std::make_shared<spdlog::sinks::daily_file_sink_mt>(filePath, hour,
                                                                            minute);
                setLevelPattern(logSinkConfig, dailyFileSink);
                sinks.push_back(dailyFileSink);
                break;
            }
            case LoggerSinkConfiguration::TcpSink: {
                spdlog::sinks::tcp_sink_config tcpSinkConfig(
                        logSinkConfig.getConnection(), logSinkConfig.getPort());
                std::shared_ptr<spdlog::sinks::sink> tcpSink =
                        std::make_shared<spdlog::sinks::tcp_sink_mt>(tcpSinkConfig);
                setLevelPattern(logSinkConfig, tcpSink);
                sinks.push_back(tcpSink);
                break;
            }
        }
    }
    auto logger = std::make_shared<spdlog::logger>(
            m_processName->getProcessName(), sinks.begin(), sinks.end());
    std::string loggerLevel =
            loggerConfigEntry->getLoggerConfiguration()->getLevel();
    {
        if (loggerLevel == "critical") {
            logger->set_level(spdlog::level::critical);
            logger->flush_on(spdlog::level::critical);
        } else if (loggerLevel == "warn") {
            logger->set_level(spdlog::level::warn);
            logger->flush_on(spdlog::level::warn);
        } else if (loggerLevel == "err") {
            logger->set_level(spdlog::level::err);
            logger->flush_on(spdlog::level::err);
        } else if (loggerLevel == "info") {
            logger->set_level(spdlog::level::info);
            logger->flush_on(spdlog::level::info);
        } else if (loggerLevel == "debug") {
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::debug);
        } else if (loggerLevel == "trace") {
            logger->set_level(spdlog::level::trace);
            logger->flush_on(spdlog::level::trace);
        } else if (loggerLevel == "off") {
            logger->set_level(spdlog::level::off);
            logger->flush_on(spdlog::level::off);
        }
    }
    m_logger->flush_on(spdlog::level::trace);
    m_logger->set_level(spdlog::level::trace);
    return logger;
}

LoggerService::~LoggerService() = default;

LoggerService &LoggerService::getOrCreate(
        const std::shared_ptr<ProcessName> &processInfo,
        const std::shared_ptr<Configuration> &configuration) {
    std::call_once(m_initInstanceFlag, &LoggerService::initSingleton, processInfo,
                   configuration);
    return get();
}

LoggerService &LoggerService::get() {
    if (m_instance == nullptr) {
        std::call_once(m_initInstanceFlag, &LoggerService::initSingleton2);
    }
    return *m_instance;
}

void LoggerService::initSingleton(
        const std::shared_ptr<ProcessName> &processInfo,
        const std::shared_ptr<Configuration> &configuration) {
    m_instance = std::make_unique<LoggerService>(processInfo, configuration);
}

void LoggerService::initSingleton2() {
    m_instance = std::make_unique<LoggerService>();
}