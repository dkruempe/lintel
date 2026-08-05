#include "base_library/core/configuration/LoggerPathConfiguration.h"

LoggerPathConfiguration::LoggerPathConfiguration(std::filesystem::path path,
                                                 bool createSubDirectories)
        : m_path(std::move(path)), m_createSubDirectories(createSubDirectories) {}

const std::filesystem::path &LoggerPathConfiguration::getPath() const {
    return m_path;
}

bool LoggerPathConfiguration::isCreateSubDirectories() const {
    return m_createSubDirectories;
}
