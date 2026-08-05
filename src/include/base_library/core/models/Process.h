#ifndef CPP_SYSTEM_LIBRARY_PROCESS_H
#define CPP_SYSTEM_LIBRARY_PROCESS_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base_library/core/utils/UUID.h"

/**
 * Represents a managed process with lifecycle callbacks and auto-restart capability.
 */
class Process {
private:
    // UUID
    std::string m_id = UUID::generate();
    // process
    std::filesystem::path m_path;
    std::vector<std::string> m_args;
    // configuration
    bool m_autoRestart = false;
    int32_t m_restarts = 0;
    int m_maxAutoRestarts = -1;
    // events
    std::shared_ptr<std::function<void(const Process &)>> m_onStart = nullptr;
    std::shared_ptr<std::function<void(const Process &)>> m_onStop = nullptr;
    std::shared_ptr<std::function<void(const Process &)>> m_onRestart = nullptr;
    std::shared_ptr<std::function<void(const Process &)>> m_onTerminate = nullptr;
    std::shared_ptr<std::function<void(const Process &)>> m_onFinish = nullptr;

public:
    /**
     * Constructor.
     * @param path path to the executable
     * @param args command-line arguments
     */
    explicit Process(std::filesystem::path path, std::vector<std::string> args);

    /** @param onStart callback invoked when the process starts */
    void addOnStartEvent(
            std::shared_ptr<std::function<void(const Process &)>> onStart);

    /** @param onStop callback invoked when the process stops */
    void addOnStopEvent(
            std::shared_ptr<std::function<void(const Process &)>> onStop);

    /** @param onRestart callback invoked when the process restarts */
    void addOnRestartEvent(
            std::shared_ptr<std::function<void(const Process &)>> onRestart);

    /** @param onTerminate callback invoked when the process is terminated */
    void addOnTerminateEvent(
            std::shared_ptr<std::function<void(const Process &)>> onTerminate);

    /** @param onFinish callback invoked when the process finishes */
    void addOnFinishEvent(
            std::shared_ptr<std::function<void(const Process &)>> onFinish);

    /**
     * Enable auto-restart for the process.
     * @param maxAutoRestarts maximum number of restarts (-1 for unlimited)
     */
    void enableAutoStart(int maxAutoRestarts = -1);

    /** Disable auto-restart for the process. */
    void disableAutoStart();

    /** Increment the restart counter. */
    void increaseRestarts();

    /** @return current number of restarts */
    [[nodiscard]] int currentRestarts() const;

    /** @return process UUID */
    [[nodiscard]] const std::string &getId() const;

    /** @return path to the executable */
    [[nodiscard]] const std::filesystem::path &getPath() const;

    /** @return true if auto-restart is enabled */
    [[nodiscard]] bool isAutoRestart() const;

    /** @return maximum number of auto-restarts */
    [[nodiscard]] int getMaxAutoRestarts() const;

    /** @return command-line arguments */
    [[nodiscard]] const std::vector<std::string> &getArgs() const;

    /** Trigger the onStart event. */
    void onStart() const;

    /** Trigger the onStop event. */
    void onStop() const;

    /** Trigger the onRestart event. */
    void onRestart() const;

    /** Trigger the onTerminate event. */
    void onTerminate() const;

    /** Trigger the onFinish event. */
    void onFinish() const;
};

#endif  // CPP_SYSTEM_LIBRARY_PROCESS_H
