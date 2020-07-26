#ifndef LOGGING_FILE_H
#define LOGGING_FILE_H

#include <filesystem>
#include <fstream>
#include <ostream>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>

/**
 * class for wrapping general operations with a normal file
 */
class File {
private:
  std::filesystem::path path;

public:
  /**
   * class for stream operations with file
   * -> reading line by line a file
   */
  class Stream {
  private:
    std::ifstream file;     // stream for operations
    bool endOfFile = false; // marks if end of file is reached

  public:
    /**
     * constructor
     * @param path of file
     */
    explicit Stream(const std::filesystem::path &path);
    /**
     * true if end of file
     * @return end of file
     */
    bool isEndOfFile() const;
    /**
     * returns read line of file
     * @return line
     */
    std::string getLine();
  };
  /**
   * constructor
   * @param path of file
   */
  explicit File(std::filesystem::path path);
  /**
   * checks if for the given path is a file
   * @return
   */
  bool isFile();
  /**
   * checks if for the given path something exists and is a file
   */
  bool exists();
  /**
   * delete file
   */
   void deleteFile();
  /**
   * reads whole file in total (not good for big files)
   * @return content of whole file in one string
   */
  std::string readFile();
  /**
   * returns name of file
   * @return name of file
   */
  std::string getName();
  /**
   * returns given path of file
   * @return path of file
   */
  std::filesystem::path getPath();
  /**
   * write content to file in total
   * @param content to be written in file
   */
  void writeToFile(const std::string &content, bool overwrite = false);
  /**
   * creates symlink of file to given path
   * @param to path where to create the sysmlink
   */
  void createSymlinkTo(const std::filesystem::path &to);
  /**
   * searches matching string of file via File::Stream
   * @param regex for searching matching strings
   * @return list of matching lines in right sequence
   */
  std::vector<std::string> matches(const std::regex &regex);
  /**
   * creates File::Stream for file
   * @return stream
   */
  File::Stream createStream();
  /**
   * to string method
   */
  friend std::ostream &operator<<(std::ostream &os, const File &file) {
    os << "path: " << file.path;
    return os;
  }
};
#endif // LOGGING_FILE_H
