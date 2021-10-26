#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H

#include <filesystem>
#include <string>

class SharedMemorySegment {
 private:
  std::filesystem::path m_path;
  std::string m_name;
  std::size_t m_size;
  bool m_isAutoExtend;
  std::size_t m_autoExtendSize;
  std::size_t m_maxSize;

 public:
  SharedMemorySegment(std::filesystem::path sharedMemoryPath, std::string name,
                      std::size_t size);

  SharedMemorySegment(std::filesystem::path sharedMemoryPath, std::string name,
                      std::size_t size, std::size_t autoExtendSize,
                      std::size_t maxSize);

  [[nodiscard]] std::filesystem::path getPath() const;
  [[nodiscard]] const std::string &getName() const;
  [[nodiscard]] size_t getSize() const;
  [[nodiscard]] bool isAutoExtend() const;
  [[nodiscard]] size_t getAutoExtendSize() const;
  [[nodiscard]] size_t getMaxSize() const;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYSEGMENT_H