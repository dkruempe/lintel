#ifndef LINTEL_LOGGERPATHCONFIGURATION_H
#define LINTEL_LOGGERPATHCONFIGURATION_H

#include <filesystem>

/** Configuration for logger log file paths */
class LoggerPathConfiguration {
private:
    /** The log file path */
    std::filesystem::path m_path;
    /** Whether to create process-specific subdirectories */
    bool m_createSubDirectories;

public:
    /** Construct a logger path configuration
     * @param path The log file path
     * @param createSubDirectories Whether to create subdirectories per process */
    LoggerPathConfiguration(std::filesystem::path path,
                            bool createSubDirectories);

    /** Get the log file path
     * @return The filesystem path */
    [[nodiscard]] const std::filesystem::path &getPath() const;

    /** Check if subdirectory creation is enabled
     * @return True if subdirectories should be created */
    [[nodiscard]] bool isCreateSubDirectories() const;
};

#endif  // LINTEL_LOGGERPATHCONFIGURATION_H
