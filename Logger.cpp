#include "Logger.h"

#include "config.h"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
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
    const std::string logString = fmt::format("[{}] times  - {}", info.logCount, info.logString);
    debug(info.function, info.file, info.line, logString);
  }
}

Logger::~Logger() = default;

void Logger::configure() {
  // I check if configuration file exists
  std::filesystem::path fileTemplate =
      std::string(CONFIG_DIRECTORY) + "/" + std::string(templateConfig);
  std::filesystem::path fileConfig =
      std::string(CONFIG_DIRECTORY) + "/" + processName + "_log4cxx.xml";

  bool foundConfig = std::filesystem::exists(fileConfig);
  bool foundTemplate = std::filesystem::exists(fileTemplate);
  bool createError = false;

  if (!foundConfig && foundTemplate) {
    std::string content = readFile(fileTemplate);
    auto found = content.find(templateMarker);
    if (found != std::string::npos) {
      content.replace(found, std::string(templateMarker).length(),
                      std::string(LOG_DIRECTORY) + std::string("/") +
                          processName + std::string(".log"));
      writeToFile(fileConfig, content);
    } else {
      createError = true;
    }
  }

  // II load configuration file or load basic configuration if no configuration
  // file found
  if (!foundTemplate || createError) {
    if (!foundTemplate) {
      std::cerr << "Template not found" << std::endl;
    } else {
      std::cerr << "Parse error with template config" << std::endl;
    }
    log4cxx::BasicConfigurator::configure();
  } else {
    std::cout << "Template found and file created" << std::endl;
    log4cxx::xml::DOMConfigurator::configureAndWatch(fileConfig.string(),
                                                     30000);
  }
}

std::string Logger::readFile(const std::filesystem::path &path) {
  std::ifstream file(path.string());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
void Logger::writeToFile(const std::filesystem::path &path,
                         const std::string &content) {
  std::ofstream out(path.string());
  out << content;
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
