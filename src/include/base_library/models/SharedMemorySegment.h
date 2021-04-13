#ifndef CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENT_H
#define CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENT_H

#include <filesystem>
#include <string>

class SharedMemorySegment {
private:
  std::filesystem::path sharedMemoryPath;
  static constexpr std::string_view fileEnding = ".bin";
  std::string name;
  std::size_t size;

public:
  SharedMemorySegment(std::filesystem::path sharedMemoryPath, std::string name,
                      std::size_t size);

  [[nodiscard]] std::filesystem::path getPath() const;

  [[nodiscard]] const std::string &getName() const;
  [[nodiscard]] size_t getSize() const;
};

#endif // CPP_SYSTEM_LIBRARY_SHAREDMEMORYSEGMENT_H