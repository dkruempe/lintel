#pragma once
#include <exception>
#include <filesystem>
#include <fmt/format.h>

class FileServiceFileExists : public std::exception {
private:
  const std::filesystem::path path;
  const std::string message;

public:
  explicit FileServiceFileExists(const std::filesystem::path &path)
      : path(path),
        message(fmt::format("No Operation possible bc. {} exists", path)) {}

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
  [[nodiscard]] const std::filesystem::path &getPath() const { return path; }
};