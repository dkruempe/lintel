#include <base_library/core/services/DirectoryService.h>
DirectoryService::DirectoryService(std::filesystem::path path)
    : m_path(std::move(path)) {}

bool DirectoryService::exists() {
  return std::filesystem::is_directory(m_path) && std::filesystem::exists(m_path);
}

std::string DirectoryService::getName() { return m_path.filename(); }

std::filesystem::path DirectoryService::getPath() { return m_path; }

bool DirectoryService::createDirectory() {
  return std::filesystem::create_directory(m_path);
}

bool DirectoryService::createDirectories() {
  return std::filesystem::create_directories(m_path);
}
