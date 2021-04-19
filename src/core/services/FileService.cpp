#include "base_library/core/services/FileService.h"

#include "base_library/core/exceptions/FileServiceFileExists.h"
#include "base_library/core/exceptions/FileServiceIsNotFileException.h"

FileService::Stream::Stream(const std::filesystem::path &path)
    : file(path.string()) {}

bool FileService::Stream::isEndOfFile() const { return endOfFile; }

std::string FileService::Stream::getLine() {
  std::string line;
  endOfFile = !std::getline(file, line);
  return line;
}

FileService::FileService(std::filesystem::path path) : path(std::move(path)) {}

bool FileService::isFile() { return std::filesystem::is_regular_file(path); }

bool FileService::exists() { return std::filesystem::exists(path) && isFile(); }

std::string FileService::readFile() {
  std::ifstream file(path.string());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string FileService::getName() { return path.filename(); }

std::filesystem::path FileService::getPath() { return path; }

void FileService::writeToFile(const std::string &content,
                              bool overwrite /* default = false */) {
  std::ofstream out;
  if (overwrite) {
    out = std::ofstream(path, std::ofstream::trunc | std::ofstream::out);
  } else {
    out = std::ofstream(path);
  }
  out << content;
}

void FileService::createSymlinkTo(const std::filesystem::path &to) {
  return std::filesystem::create_symlink(path, to);
}

std::vector<std::string> FileService::matches(const std::regex &regex) {
  FileService::Stream stream(path.string());
  std::vector<std::string> matches;
  while (!stream.isEndOfFile()) {
    const std::string &line = stream.getLine();
    if (std::regex_match(line, regex)) {
      matches.push_back(line);
    }
  }
  return matches;
}

FileService::Stream FileService::createStream() {
  if (!isFile()) {
    throw FileServiceIsNotFileException(path);
  }
  return Stream(path);
}
void FileService::deleteFile() {
  if (!exists()) {
    return;
  }
  std::filesystem::remove(path);
}
std::size_t FileService::getSize() { return std::filesystem::file_size(path); }
void FileService::createFile(std::size_t sizeOfFile) {
  if (exists()) {
    throw FileServiceFileExists(path);
  }
  std::filebuf fbuf;
  fbuf.open(path, std::ios_base::in | std::ios_base::out |
                      std::ios_base::trunc | std::ios_base::binary);
  // Set the size
  fbuf.pubseekoff(sizeOfFile - 1, std::ios_base::beg);
  fbuf.sputc(0);
}
