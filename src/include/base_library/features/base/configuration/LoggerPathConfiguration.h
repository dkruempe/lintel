#ifndef CPP_BASE_LIBRARY_LOGGERPATHCONFIGURATION_H
#define CPP_BASE_LIBRARY_LOGGERPATHCONFIGURATION_H

#include <filesystem>

class LoggerPathConfiguration {
private:
    std::filesystem::path m_path;
    bool m_createSubDirectories;

public:
    LoggerPathConfiguration(std::filesystem::path path,
                            bool createSubDirectories);

    [[nodiscard]] const std::filesystem::path &getPath() const;

    [[nodiscard]] bool isCreateSubDirectories() const;
};

#endif  // CPP_BASE_LIBRARY_LOGGERPATHCONFIGURATION_H
