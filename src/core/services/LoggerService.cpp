#include "base_library/core/services/LoggerService.h"

#include <fmt/format.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>

#include <algorithm>
#include <iostream>

#include "base_library/config.h"
#include "base_library/core/exceptions/LoggerServiceNotInitialized.h"
#include "base_library/core/services/DirectoryService.h"
#include "base_library/core/services/FileService.h"
// initialization of static variables
LoggerService *LoggerService::m_instance = nullptr;
std::once_flag LoggerService::m_initInstanceFlag;

LoggerService::LoggerService(const std::string &processName)
    : m_processName(processName),
      m_logger(log4cxx::Logger::getLogger(processName)) {
  configure(false);
}

LoggerService::LoggerService()
    : m_processName("DUMMY"),
      m_logger(log4cxx::Logger::getLogger(m_processName)) {
  configure(true);
}

LoggerService::~LoggerService() = default;

void LoggerService::configure(bool consoleOnly) {
  bool createError = false;

  if (consoleOnly) {
    log4cxx::BasicConfigurator::configure();
    return;
  }

  FileService templateFile(std::string(CONFIG_DIRECTORY) + "/" +
                           std::string(m_templateConfig));
  FileService configFile(std::string(CONFIG_DIRECTORY) + "/" + m_processName +
                         "_log4cxx.xml");
  DirectoryService logDir(std::string(LOG_DIRECTORY));
  if (!logDir.exists()) {
    logDir.createDirectories();
  }
  if (!configFile.exists() && templateFile.exists()) {
    std::string content = templateFile.readFile();
    auto found = content.find(m_templateMarker);
    if (found != std::string::npos) {
      content.replace(found, std::string(m_templateMarker).length(),
                      std::string(LOG_DIRECTORY) + std::string("/") +
                          m_processName + std::string(".log"));
      configFile.writeToFile(content, false);
    } else {
      createError = true;
    }
  }

  // II load configuration file or load basic configuration if no configuration
  // file found
  if (!templateFile.exists() || createError) {
    if (!templateFile.exists()) {
      std::cerr << "Template not found" << std::endl;
    } else {
      std::cerr << "Parse error with template config" << std::endl;
    }
    log4cxx::BasicConfigurator::configure();
  } else {
    log4cxx::xml::DOMConfigurator::configureAndWatch(
        configFile.getPath().string(), 30000);
  }
}

LoggerService &LoggerService::getOrCreate(const std::string &argv) {
  std::string processName = std::filesystem::path(argv).filename();
  if (!processName.empty()) {
    std::call_once(m_initInstanceFlag, &LoggerService::initSingleton,
                   processName);
  }
  return get();
}

LoggerService &LoggerService::get() {
  if (m_instance == nullptr) {
    std::call_once(m_initInstanceFlag, &LoggerService::init);
    LOG_FATAL(
        "LoggerService not initialized. Please call Marco DECLARE_LOGGER");
  }
  return *m_instance;
}
void LoggerService::init() { m_instance = new LoggerService(); }
void LoggerService::initSingleton(const std::string &processName) {
  m_instance = new LoggerService(processName);
}
