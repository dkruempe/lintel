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
class FileService {
 private:
  std::filesystem::path m_path;

 public:
  /**
   * class for stream operations with file
   * -> reading line by line a file
   */
  class Stream {
   private:
    std::ifstream m_file;      // stream for operations
    bool m_endOfFile = false;  // marks if end of file is reached

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
  explicit FileService(std::filesystem::path path);
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
   * returns size of file
   */
  std::size_t getSize();
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
   * creates empty with give file size
   */
  void createFile(std::size_t sizeOfFile);
  /**
   * creates symlink of file to given path
   * @param to path where to create the sysmlink
   */
  void createSymlinkTo(const std::filesystem::path &to);
  /**
   * searches matching string of file via FileService::Stream
   * @param regex for searching matching strings
   * @return list of matching lines in right sequence
   */
  std::vector<std::string> matches(const std::regex &regex);
  /**
   * creates FileService::Stream for file
   * @return stream
   */
  FileService::Stream createStream();
  /**
   * to string method
   */
  friend std::ostream &operator<<(std::ostream &os, const FileService &file) {
    os << "path: " << file.m_path;
    return os;
  }
};
#endif  // LOGGING_FILE_H
