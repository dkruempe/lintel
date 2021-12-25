#ifndef CPP_BASE_LIBRARY_MEMORYSIZE_H
#define CPP_BASE_LIBRARY_MEMORYSIZE_H

#include <string>

class MemorySize {
 public:
  /**
   * deserialize memory size typically to byte as integer value
   * @param size as string
   * @return size as byte integer
   */
  static std::size_t deserialize(const std::string &size);
  /**
   * converts byte to memory size string
   * @param size as byte integer
   * @return size as string
   */
  static std::string serialize(std::size_t byte);
};

#endif  // CPP_BASE_LIBRARY_MEMORYSIZE_H
