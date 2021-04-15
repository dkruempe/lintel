#include <base_library/core/services/StopWatchService.h>
#include <chrono>
#include <iostream>
#include <thread>
int main(int argc, char *argv[]) {
  StopWatchService stopWatch (true);
  std::this_thread::sleep_for(std::chrono::milliseconds (100));
  stopWatch.stop();
  std::cout << stopWatch << std::endl;
  return 0;
}