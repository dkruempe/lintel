#include "base_library/Logger.h"

#include "base_library/config.h"

#include <algorithm>
#include <base_library/File.h>
#include <fmt/format.h>
#include <iostream>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
// initialization of static variables
Logger *Logger::instance = nullptr;
std::once_flag Logger::initInstanceFlag;

Logger::Logger(const std::string &processName)
    : processName(processName),
      logger(log4cxx::Logger::getLogger(processName)) {
  configure();
  std::function<void()> call = [&]() { logRepeatLog(); };
  scheduler.schedule_at_fixed_rate(std::chrono::seconds(30),
                                   std::chrono::seconds(30), call);
}

void Logger::logRepeatLog() {
  std::map<std::string, LOG_INFO> tmp;
  {
    std::unique_lock<std::mutex> lock(mutex);
    for (auto &iter : repeatLogs) {
      tmp.insert(iter);
    }
    repeatLogs.clear();
  }
  for (auto &[key, info] : tmp) {
    const std::string logString =
        fmt::format("[{}] times  - {}", info.logCount, info.logString);
    debug(info.function, info.file, info.line, logString);
  }
}

Logger::~Logger() = default;

void Logger::configure() {
  bool createError = false;

  File templateFile(std::string(CONFIG_DIRECTORY) + "/" +
                    std::string(templateConfig));
  File configFile(std::string(CONFIG_DIRECTORY) + "/" + processName +
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

Logger &Logger::getOrCreate(const std::string &processName) {
  if (!processName.empty()) {
    std::call_once(initInstanceFlag, &Logger::initSingleton, processName);
  }
  return get();
}

Logger &Logger::get() {
  if (instance == nullptr) {
    throw std::runtime_error(
        "Logger not initialized. Please call Marco DECLARE_LOGGER");
  }
  return *instance;
}
void Logger::initSingleton(const std::string &processName) {
  instance = new Logger(processName);
}
