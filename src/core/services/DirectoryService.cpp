#include <base_library/core/services/DirectoryService.h>
DirectoryService::DirectoryService(std::filesystem::path path)
    : path(std::move(path)) {}

bool DirectoryService::exists() {
  return std::filesystem::is_directory(path) && std::filesystem::exists(path);
}

std::string DirectoryService::getName() { return path.filename(); }

std::filesystem::path DirectoryService::getPath() { return path; }

bool DirectoryService::createDirectory() {
  return std::filesystem::create_directory(path);
}

bool DirectoryService::createDirectories() {
  return std::filesystem::create_directories(path);
}
