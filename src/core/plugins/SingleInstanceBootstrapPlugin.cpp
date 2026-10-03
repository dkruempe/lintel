#include "base_library/core/plugins/SingleInstanceBootstrapPlugin.h"

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentEntry.h"

/** Operating system file lock of the single instance plugin. */
class SingleInstanceBootstrapPlugin::Lock {
public:
    /** @param path path of the lock file */
    explicit Lock(const std::filesystem::path &path) : m_lock(path.c_str()) {}

    /** @return true if the exclusive lock was acquired */
    [[nodiscard]] bool tryLock() { return m_lock.try_lock(); }

private:
    boost::interprocess::file_lock m_lock;
};

SingleInstanceBootstrapPlugin::~SingleInstanceBootstrapPlugin() = default;

SingleInstanceBootstrapPlugin::SingleInstanceBootstrapPlugin(
        const std::shared_ptr<DatabaseConnectionConfigurations>
        &connectionConfigurations,
        std::shared_ptr<Configuration> configuration,
        std::shared_ptr<ProcessName> processName)
        : m_connectionEntry(connectionConfigurations->ofDefault()),
          m_configuration(std::move(configuration)),
          m_processName(std::move(processName)) {}

void SingleInstanceBootstrapPlugin::acquireLock() {
    std::filesystem::path lockDirectory =
            std::filesystem::temp_directory_path();
    auto segmentEntries =
            m_configuration->configurationOf<SharedMemorySegmentComponent>();
    for (const auto &entry: segmentEntries) {
        const auto segmentEntry =
                std::static_pointer_cast<SharedMemorySegmentEntry>(entry);
        const auto &sharedPath = segmentEntry->getSharedMemoryPath();
        if (sharedPath == nullptr || sharedPath->empty()) {
            continue;
        }
        lockDirectory = *sharedPath;
        break;
    }
    std::error_code error;
    std::filesystem::create_directories(lockDirectory, error);
    const std::filesystem::path lockFile =
            lockDirectory /
            (m_processName->getProcessName() + ".lock");
    if (!std::filesystem::exists(lockFile)) {
        std::ofstream(lockFile) << "";
    }
    auto fileLock = std::make_unique<Lock>(lockFile);
    if (!fileLock->tryLock()) {
        throw std::runtime_error(
                "another instance of process '" + m_processName->getProcessName()
                + "' is already running (lock file '" + lockFile.string()
                + "')");
    }
    m_fileLock = std::move(fileLock);
    LOG_INFO("acquired single instance lock '{}'", lockFile.string());
}

void SingleInstanceBootstrapPlugin::onStart() {
    if (m_connectionEntry == nullptr) {
        LOG_INFO("no default database connection - skip single instance lock");
        return;
    }
    try {
        acquireLock();
    } catch (const boost::interprocess::interprocess_exception &exception) {
        throw std::runtime_error(
                "cannot acquire single instance lock: "
                + std::string(exception.what()));
    }
}

BootstrapSequence SingleInstanceBootstrapPlugin::getPriority() {
    return BootstrapSequence::SingleInstance;
}
