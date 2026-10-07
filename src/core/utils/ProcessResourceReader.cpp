#include "lintel/core/utils/ProcessResourceReader.h"

#include <algorithm>

#if defined(__linux__)
#include <fstream>
#include <sstream>
#include <string>
#elif defined(__APPLE__)
#include <libproc.h>
#include <string>
#endif

namespace {

constexpr double kClockTicksPerSecond = 100.0;

}// namespace

ProcessResourceData ProcessResourceReader::readOf(pid_t pid,
  const std::optional<ProcessResourceData> &previous)
{
  ProcessResourceData data;
  if (pid < 0) { return data; }

#if defined(__linux__)
  // /proc/<pid>/stat: the comm field may contain spaces/parentheses, so find
  // the last ')' and parse the numerically-indexed fields that follow.
  std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
  if (!statFile.is_open()) { return data; }
  std::string line;
  if (!std::getline(statFile, line)) { return data; }
  const std::size_t closeParen = line.rfind(')');
  if (closeParen == std::string::npos) { return data; }
  std::istringstream iss(line.substr(closeParen + 1));
  // The token right after ')' corresponds to field 3 (state); utime is field
  // 14, stime is field 15 and starttime is field 22 in the full field list.
  unsigned long long utime = 0, stime = 0, starttime = 0;
  std::string token;
  int fieldIndex = 2;
  while (iss >> token) {
    ++fieldIndex;
    if (fieldIndex == 14) {
      utime = std::stoull(token);
    } else if (fieldIndex == 15) {
      stime = std::stoull(token);
    } else if (fieldIndex == 22) {
      starttime = std::stoull(token);
    }
  }
  data.cpuTicksTotal = utime + stime;
  const unsigned long long hertz = static_cast<unsigned long long>(sysconf(_SC_CLK_TCK));
  if (starttime != 0 && hertz != 0) {
    const long long startSeconds = static_cast<long long>(starttime / hertz);
    // subtract the process start time (seconds since boot) from the current
    // boot duration read from /proc/uptime to obtain the process uptime.
    long long bootSeconds = 0;
    std::ifstream uptimeFile("/proc/uptime");
    if (uptimeFile.is_open()) { uptimeFile >> bootSeconds; }
    const long long uptime = bootSeconds > startSeconds ? bootSeconds - startSeconds : 0;
    data.uptime = std::chrono::seconds(uptime);
  }
  data.valid = true;

  // Resident memory: /proc/<pid>/status VmRSS in kB.
  std::ifstream statusFile("/proc/" + std::to_string(pid) + "/status");
  if (statusFile.is_open()) {
    std::string statusLine;
    while (std::getline(statusFile, statusLine)) {
      if (statusLine.rfind("VmRSS:", 0) != 0) { continue; }
      std::istringstream valueStream(statusLine.substr(7));
      unsigned long long kb = 0;
      valueStream >> kb;
      data.memoryBytes = kb * 1024ULL;
      break;
    }
  }
#elif defined(__APPLE__)
  struct proc_taskinfo taskInfo{};
  const int size = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &taskInfo, sizeof(taskInfo));
  if (size != sizeof(taskInfo)) { return data; }
  data.cpuTicksTotal = static_cast<uint64_t>(taskInfo.pti_total_user + taskInfo.pti_total_system);
  data.memoryBytes = static_cast<uint64_t>(taskInfo.pti_resident_size);
  data.valid = true;
#else
  // unsupported platform: data stays invalid
  static_cast<void>(previous);
#endif

  // Compute a CPU percentage delta between the two samples.
  if (previous.has_value() && previous->valid && data.cpuTicksTotal.has_value()
      && previous->cpuTicksTotal.has_value()) {
    const auto elapsed = std::chrono::duration<double>(data.sampleTime - previous->sampleTime).count();
    if (elapsed > 0.0) {
      const double ticksDelta = static_cast<double>(data.cpuTicksTotal.value() - previous->cpuTicksTotal.value());
      data.cpuPercent = std::max(0.0, ticksDelta / (kClockTicksPerSecond * elapsed) * 100.0);
    }
  }
  return data;
}
