#include <base_library/core/services/SignalService.h>

#include <iostream>

int main(int argc, char *argv[]) {
  SignalService::registerHooks({SIGINT});
  std::cout << "REGISTERED HOOKS => wait for user interrupt" << std::endl;
  SignalService::waitForUserInterrupt();
  std::cout << "SHUTDOWN" << std::endl;
  return 0;
}