#include <base_library/services/LoggerService.h>
#include <chrono>
#include <csignal>
#include <libgen.h>
#include <thread>
#include <utility>

class ExampleClass {
private:
  std::string information;

public:
  [[maybe_unused]] explicit ExampleClass(std::string information)
      : information(std::move(information)) {}

  void hello() {
    LOG_DEBUG("Hello World {} {}", 4711, information);
    LOG_TRACE("Hello World {}", 4712);
    LOG_WARN("Hello World {}", 4713);
    LOG_ERROR("Hello World {}", 4714);
    LOG_FATAL("Hello World {}", 4715);
  }
};

volatile bool running = true;

void receiveSignal(int signal) {
  switch (signal) {
  case SIGINT:
  case SIGHUP:
    running = false;
    break;
  default:
    break;
  }
}

/*
 * input {
 *   tcp {
 *     port => 4560
 *     codec => json
 *   }
 * }
 * filter {
 *   date {
 *     timezone=>"UTC"
 *     match=>["timestamp", "UNIX_MS"]
 *     target=>"@timestamp1"
 *   }
 * }
 * output {
 *   file {
 *     path => "/var/log/file_logs/app_%{logger}.log"
 *     codec => line { format => "[%{@timestamp1}] [%{logger}] [%{@severity}] [%{file}:%{line}] %{message}"}
 *  }
 * }
*/

int main(int argc, char *argv[]) {
  ExampleClass exampleClass(">Information<");
  signal(SIGINT, receiveSignal);
  signal(SIGHUP, receiveSignal);
  DECLARE_LOGGER(basename(argv[0]));
  exampleClass.hello();

  LOG_INFO("Hello World {} {}.{}.{} und {}", "Ich liebe dich Anna!", 3, 9, 2016,
           "04.10.2019");
  int count = 0;
  log4cxx::MDC::put("rce", "2");
  log4cxx::MDC::put("unit", "4711");
  while (running) {
    LOG_TRACE("count={}", count++);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}