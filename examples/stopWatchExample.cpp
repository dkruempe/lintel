#include <iostream>
#include <chrono>
#include <thread>
#include <base_library/services/StopWatchService.h>
int main(int argc, char *argv[]) {
  StopWatchService stopWatch (true);
  std::this_thread::sleep_for(std::chrono::milliseconds (100));
  stopWatch.stop();
  std::cout << stopWatch << std::endl;
  return 0;
}