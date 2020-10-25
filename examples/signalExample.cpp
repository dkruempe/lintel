#include <iostream>
#include <base_library/services/SignalService.h>

int main(int argc, char *argv[]) {
  SignalService::registerHooks({SIGINT});
  std::cout << "REGISTERED HOOKS => wait for user interrupt" << std::endl;
  SignalService::waitForUserInterrupt();
  std::cout << "SHUTDOWN" << std::endl;
  return 0;
}