#include <base_library/Directory.h>
Directory::Directory(std::filesystem::path path) : path(std::move(path)) {}

bool Directory::exists() {
  return std::filesystem::is_directory(path) && std::filesystem::exists(path);
}

std::string Directory::getName() { return path.filename(); }

std::filesystem::path Directory::getPath() { return path; }

bool Directory::createDirectory() {
  return std::filesystem::create_directory(path);
}

bool Directory::createDirectories() {
  return std::filesystem::create_directories(path);
}
