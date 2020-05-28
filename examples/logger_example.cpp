#include "Logger.h"
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

int main(int argc, char *argv[]) {
  ExampleClass exampleClass(">Information<");
  signal(SIGINT, receiveSignal);
  signal(SIGHUP, receiveSignal);
  DECLARE_LOGGER(basename(argv[0]));
  exampleClass.hello();

  LOG_INFO("Hello World {} {}.{}.{} und {}", "Ich liebe dich Anna!", 3, 9, 2016,
           "04.10.2019");
  int count = 0;
  while (running) {
    LOG_REPEAT("count++");
    LOG_TRACE("count={}", count++);
    LOG_REPEAT("count++");
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}