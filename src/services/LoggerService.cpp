#include "base_library/services/LoggerService.h"

#include "base_library/config.h"

#include <algorithm>
#include <base_library/services/FileService.h>
#include <fmt/format.h>
#include <iostream>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
// initialization of static variables
LoggerService *LoggerService::instance = nullptr;
std::once_flag LoggerService::initInstanceFlag;

LoggerService::LoggerService(const std::string &processName)
    : processName(processName),
      logger(log4cxx::Logger::getLogger(processName)) {
  configure();
}

LoggerService::~LoggerService() = default;

void LoggerService::configure() {
  bool createError = false;

  FileService templateFile(std::string(CONFIG_DIRECTORY) + "/" +
                    std::string(templateConfig));
  FileService configFile(std::string(CONFIG_DIRECTORY) + "/" + processName +
                  "_log4cxx.xml");

  if (!configFile.exists() && templateFile.exists()) {
    std::string content = templateFile.readFile();
    auto found = content.find(templateMarker);
    if (found != std::string::npos) {
      content.replace(found, std::string(templateMarker).length(),
                      std::string(LOG_DIRECTORY) + std::string("/") +
                          processName + std::string(".log"));
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

LoggerService &LoggerService::getOrCreate(const std::string &processName) {
  if (!processName.empty()) {
    std::call_once(initInstanceFlag, &LoggerService::initSingleton, processName);
  }
  return get();
}

LoggerService &LoggerService::get() {
  if (instance == nullptr) {
    throw std::runtime_error(
        "LoggerService not initialized. Please call Marco DECLARE_LOGGER");
  }
  return *instance;
}
void LoggerService::initSingleton(const std::string &processName) {
  instance = new LoggerService(processName);
}
