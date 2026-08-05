#pragma once

#include <exception>
#include <filesystem>

/** Exception thrown when a path is expected to be a file but is not */
class FileServiceIsNotFileException : public std::exception {
private:
    const std::filesystem::path m_path;
    const std::string m_message;

public:
    /** @param path the path that is not a file */
    explicit FileServiceIsNotFileException(const std::filesystem::path &path)
            : m_path(path),
              m_message("No Operation possible bc. " + path.string() +
                        " is not a file") {}

    [[nodiscard]] const char *what() const noexcept override {
        return m_message.c_str();
    }

    /** @return the file path */
    [[nodiscard]] const std::filesystem::path &getPath() const { return m_path; }
};
