#include <base_library/config.h>
#include <base_library/core/services/FileService.h>

#include <iostream>

int main(int argc, char *argv[]) {
  FileService file(std::string(CONFIG_DIRECTORY) + "/template_log4cxx.xml");
  std::cout << "test stream \n";
  {
    FileService::Stream stream = file.createStream();
    while (!stream.isEndOfFile()) {
      auto line = stream.getLine();
      std::cout << line << "\n";
    }
  }
  std::cout << "test matching function \n";
  {
    const std::regex regex(".*template.*");
    auto vector = file.matches(regex);
    for (const auto &iter : vector) {
      std::cout << iter << "\n";
    }
  }
  return 0;
}