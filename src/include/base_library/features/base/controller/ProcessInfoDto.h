#ifndef CPP_BASE_LIBRARY_PROCESSINFODTO_H
#define CPP_BASE_LIBRARY_PROCESSINFODTO_H

#include <memory>
#include <string>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/base/models/ProcessInfo.h"

class ProcessInfoDto : public JsonSerializable {
private:
    // process
    std::string m_id;
    std::filesystem::path m_path;
    std::vector<std::string> m_args;
    bool m_autoRestart;
    int32_t m_restarts;
    int32_t m_maxAutoRestarts;
    // process state
    boost::process::v1::pid_t m_processId;
    bool m_isRunning;
    int32_t m_exitCode;
    // process group
    std::string m_groupName;
    std::string m_groupId;

    static struct Shapes {
        const char *const ID = "id";
        const char *const PATH = "path";
        const char *const ARG = "arg";
        const char *const ARGS = "args";
        const char *const AUTO_RESTART = "auto_restart";
        const char *const RESTARTS = "restarts";
        const char *const MAX_RESTARTS = "max_restarts";
        const char *const PROCESS_ID = "process_id";
        const char *const RUNS = "runs";
        const char *const EXIT_CODE = "exit_code";
        const char *const GROUP_NAME = "group_name";
        const char *const GROUP_ID = "group_id";
    } m_shape;

public:
    explicit ProcessInfoDto(const ProcessInfo &processInfo);

    ProcessInfoDto() = default;

    [[nodiscard]] const std::string &getId() const;

    [[nodiscard]] const std::filesystem::path &getPath() const;

    [[nodiscard]] const std::vector<std::string> &getArgs() const;

    [[nodiscard]] bool isAutoRestart() const;

    [[nodiscard]] int32_t getRestarts() const;

    [[nodiscard]] int32_t getMaxAutoRestarts() const;

    [[nodiscard]] pid_t getProcessId() const;

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] int32_t getExitCode() const;

    [[nodiscard]] const std::string &getGroupName() const;

    [[nodiscard]] const std::string &getGroupId() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROCESSINFODTO_H