#pragma once
#include <fmt/format.h>

#include <exception>
#include <filesystem>

class FileServiceFileExists : public std::exception {
 private:
  const std::filesystem::path path;
  const std::string message;

 public:
  explicit FileServiceFileExists(const std::filesystem::path &path)
      : path(path),
        message(fmt::format("No Operation possible bc. {} exists",
                            path.string())) {}

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
  [[nodiscard]] const std::filesystem::path &getPath() const { return path; }
};