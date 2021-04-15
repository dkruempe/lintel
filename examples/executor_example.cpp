#include <base_library/features/base/services/ExecutorService.h>
#include <iostream>

int main(int argc, char *argv[]) {
  ExecutorService executorService;
  executorService.execute([]() {
    std::printf("Hello World\n");
    std::cout << "Hello World" << std::endl;
  });
  executorService.execute(
      []() { std::cout << "Ich liebe dich Anna" << std::endl; });
  executorService.execute(
      []() { std::cout << "Ich liebe dich meine Frau <3 !" << std::endl; });
  return 0;
}