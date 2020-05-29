#include "Scheduler.h"
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/log4cxx.h>
#include <log4cxx/logger.h>
#include <map>
#include <vector>

class MultiLogger {
private:
  std::map<std::string, log4cxx::LoggerPtr> loggers;

public:
  explicit MultiLogger(const std::vector<std::string> &loggerNames) {
    for (const auto &loggerName : loggerNames) {
      loggers.insert({loggerName, log4cxx::Logger::getLogger(loggerName)});
    }
    log4cxx::BasicConfigurator::configure();
  }

  void log(const std::string &loggerName, const std::string &message) {
    auto found = loggers.find(loggerName);
    if (found == loggers.end()) {
      // no logger => destroy log event
      return;
    }
    found->second->debug(message);
  }
};

int main(int argc, char *argv[]) {
  std::vector<std::string> loggerNames = {"anna", "example_user"};
  MultiLogger multiLogger(loggerNames);
  multiLogger.log("anna", "test");
  multiLogger.log("example_user", "test2");

  Scheduler scheduler(10);
  return 0;
}