#pragma once

#include <fmt/format.h>

#include <exception>
#include <filesystem>

class FileServiceIsNotFileException : public std::exception {
private:
    const std::filesystem::path m_path;
    const std::string m_message;

public:
    explicit FileServiceIsNotFileException(const std::filesystem::path &path)
            : m_path(path),
              m_message(fmt::format("No Operation possible bc. {} is not a file",
                                    path.string())) {}

    [[nodiscard]] const char *what() const noexcept override {
        return m_message.c_str();
    }

    [[nodiscard]] const std::filesystem::path &getPath() const { return m_path; }
};