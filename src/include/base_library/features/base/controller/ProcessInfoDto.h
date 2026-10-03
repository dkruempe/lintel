#ifndef CPP_BASE_LIBRARY_PROCESSINFODTO_H
#define CPP_BASE_LIBRARY_PROCESSINFODTO_H

#include <memory>
#include <optional>
#include <string>
#include <unistd.h>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/core/utils/ProcessResourceReader.h"
#include "base_library/features/base/models/ProcessInfo.h"

/** DTO representing detailed process information */
class ProcessInfoDto : public JsonSerializable
{
private:
  /** Process ID */
  std::string m_id;
  /** Path to the process executable */
  std::filesystem::path m_path;
  /** Command line arguments */
  std::vector<std::string> m_args;
  /** Whether auto-restart is enabled */
  bool m_autoRestart;
  /** Current restart count */
  int32_t m_restarts;
  /** Maximum allowed restarts */
  int32_t m_maxAutoRestarts;
  /** Operating system process ID */
  pid_t m_processId;
  /** Whether the process is currently running */
  bool m_isRunning;
  /** The exit code of the process */
  int32_t m_exitCode;
  /** Whether the exit code is meaningful (child was waited on) */
  bool m_exitCodeValid = true;
  /** The group name this process belongs to */
  std::string m_groupName;
  /** The group ID this process belongs to */
  std::string m_groupId;
  /** Process uptime in seconds */
  std::optional<int64_t> m_uptimeSeconds;
  /** CPU usage in percent (delta-based) */
  std::optional<double> m_cpuPercent;
  /** Resident memory usage in bytes */
  std::optional<uint64_t> m_memoryBytes;
  /** Whether resource data could be read */
  bool m_resourceDataValid = false;

  /** JSON field name constants */
  static struct Shapes
  {
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
    const char *const EXIT_CODE_VALID = "exit_code_valid";
    const char *const GROUP_NAME = "group_name";
    const char *const GROUP_ID = "group_id";
    const char *const UPTIME_SECONDS = "uptime_seconds";
    const char *const CPU_PERCENT = "cpu_percent";
    const char *const MEMORY_BYTES = "memory_bytes";
    const char *const RESOURCES_VALID = "resources_valid";
  } m_shape;

public:
  /** Construct from a ProcessInfo model
   * @param processInfo The source process info */
  explicit ProcessInfoDto(const ProcessInfo &processInfo);

  /** Default constructor */
  ProcessInfoDto() = default;

  /** Get the process ID
   * @return The ID */
  [[nodiscard]] const std::string &getId() const;

  /** Get the executable path
   * @return The filesystem path */
  [[nodiscard]] const std::filesystem::path &getPath() const;

  /** Get the command line arguments
   * @return Vector of argument strings */
  [[nodiscard]] const std::vector<std::string> &getArgs() const;

  /** Check if auto-restart is enabled
   * @return True if auto-restart is enabled */
  [[nodiscard]] bool isAutoRestart() const;

  /** Get the current restart count
   * @return The number of restarts */
  [[nodiscard]] int32_t getRestarts() const;

  /** Get the maximum allowed auto-restarts
   * @return The max restarts */
  [[nodiscard]] int32_t getMaxAutoRestarts() const;

  /** Get the OS process ID
   * @return The process ID */
  [[nodiscard]] pid_t getProcessId() const;

  /** Check if the process is running
   * @return True if running */
  [[nodiscard]] bool isRunning() const;

  /** Get the exit code
   * @return The exit code */
  [[nodiscard]] int32_t getExitCode() const;

  /** Check if the exit code is meaningful
   * @return True if the exit code is valid */
  [[nodiscard]] bool isExitCodeValid() const;

  /** Get the group name
   * @return The group name */
  [[nodiscard]] const std::string &getGroupName() const;

  /** Get the group ID
   * @return The group ID */
  [[nodiscard]] const std::string &getGroupId() const;

  /** Get the process uptime in seconds (if readable)
   * @return The uptime or nullopt */
  [[nodiscard]] std::optional<int64_t> getUptimeSeconds() const;

  /** Get the CPU usage in percent (if a delta is available)
   * @return The CPU percent or nullopt */
  [[nodiscard]] std::optional<double> getCpuPercent() const;

  /** Get the resident memory usage in bytes (if readable)
   * @return The memory bytes or nullopt */
  [[nodiscard]] std::optional<uint64_t> getMemoryBytes() const;

  /** Check if resource data is valid
   * @return True if resource data could be read */
  [[nodiscard]] bool isResourceDataValid() const;

  /** Serialize to JSON
   * @param writer The rapidjson writer */
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  /** Deserialize from JSON
   * @param obj The JSON value
   * @return True on success */
  bool deserialize(const rapidjson::Value &obj) override;
};

#endif// CPP_BASE_LIBRARY_PROCESSINFODTO_H