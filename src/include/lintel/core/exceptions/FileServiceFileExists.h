#pragma once

#include <exception>
#include <filesystem>
#include <format>

namespace lintel {
/** Exception thrown when an operation cannot proceed because a file already exists */
class FileServiceFileExists : public std::exception
{
private:
  const std::filesystem::path m_path;
  const std::string m_message;

public:
  /** @param path the path of the existing file */
  explicit FileServiceFileExists(const std::filesystem::path &path)
    : m_path(path), m_message(std::format("No Operation possible bc. {} exists", path.string()))
  {}

  [[nodiscard]] const char *what() const noexcept override { return m_message.c_str(); }

  /** @return the file path */
  [[nodiscard]] const std::filesystem::path &getPath() const { return m_path; }
};
}// namespace lintel
