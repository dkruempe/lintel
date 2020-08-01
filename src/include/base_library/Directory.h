#ifndef LOGGING_DIRECTORY_H
#define LOGGING_DIRECTORY_H

#include <filesystem>

/**
 * Directory class wraps default operations with the directory
 */
class Directory {
private:
  std::filesystem::path path;

public:
  /**
   * Constructor
   */
  explicit Directory(std::filesystem::path path);

  /**
   * Checks if Directory is available
   * @return true if directory exists and is directory
   */
  bool exists();
  /**
   * returns name of directory
   */
  std::string getName();
  /**
   * returns path of directory
   * @return true if successfull
   */
  std::filesystem::path getPath();
  /**
   * create single directory
   * @return true if successful
   */
  bool createDirectory();
  /**
   * creates recursive all needed directory
   * @return true if successful
   */
  bool createDirectories();
};

#endif // LOGGING_DIRECTORY_H
