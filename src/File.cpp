#include "File.h"

File::Stream::Stream(const std::filesystem::path &path) : file(path.string()) {}

bool File::Stream::isEndOfFile() const { return endOfFile; }

std::string File::Stream::getLine() {
  std::string line;
  endOfFile = !std::getline(file, line);
  return line;
}

File::File(std::filesystem::path path) : path(std::move(path)) {}

bool File::isFile() { return std::filesystem::is_regular_file(path); }

bool File::exists() { return std::filesystem::exists(path) && isFile(); }

std::string File::readFile() {
  std::ifstream file(path.string());
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string File::getName() { return path.filename(); }

std::filesystem::path File::getPath() { return path; }

void File::writeToFile(const std::string &content,
                       bool overwrite /* default = false */) {
  std::ofstream out;
  if (overwrite) {
    out = std::ofstream(path, std::ofstream::trunc | std::ofstream::out);
  } else {
    out = std::ofstream(path);
  }
  out << content;
}

void File::createSymlinkTo(const std::filesystem::path &to) {
  return std::filesystem::create_symlink(path, to);
}

std::vector<std::string> File::matches(const std::regex &regex) {
  File::Stream stream(path.string());
  std::vector<std::string> matches;
  while (!stream.isEndOfFile()) {
    const std::string &line = stream.getLine();
    if (std::regex_match(line, regex)) {
      matches.push_back(line);
    }
  }
  return matches;
}

File::Stream File::createStream() {
  if (!isFile()) {
    throw std::runtime_error(path.string() + ": is no file or does not exists");
  }
  return Stream(path);
}
void File::deleteFile() {
  if (!exists()) {
    return;
  }
  std::filesystem::remove(path);
}
