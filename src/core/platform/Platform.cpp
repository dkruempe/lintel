#include "lintel/core/platform/Platform.h"

#include <cerrno>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
// NOMINMAX and WIN32_LEAN_AND_MEAN are already defined by Platform.h, which has
// to be included first for that. OpenProcess/GetExitCodeProcess need the rest of
// <Windows.h>, which is why this lives in a translation unit instead of the
// public header.
#include <Windows.h>
#else
#include <csignal>// kill
#endif

namespace lintel {
namespace {
#if !defined(_WIN32)
  /** Split on spaces, ignoring runs of them. */
  std::vector<std::string> splitOnSpaces(const std::string &line)
  {
    std::vector<std::string> fields;
    std::size_t position = 0;
    while (position < line.size()) {
      while (position < line.size() && line[position] == ' ') { ++position; }
      const std::size_t start = position;
      while (position < line.size() && line[position] != ' ') { ++position; }
      if (position > start) { fields.emplace_back(line.substr(start, position - start)); }
    }
    return fields;
  }
#endif
}// namespace

/**
 * @param processId see Platform.h
 * @return see Platform.h
 *
 * A process that is still running is not necessarily ours: kill(pid, 0) answers
 * EPERM when it belongs to another user, which still means "exists". Only a
 * dead process answers ESRCH, and that is the case the lock recovery may
 * reclaim (see shm::SegmentLockOwner).
 */
bool processIsAlive(ProcessId processId) noexcept
{
  if (processId <= 0) { return false; }
#if defined(_WIN32)
  const HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processId));
  if (handle == nullptr) {
    // ERROR_ACCESS_DENIED means the process exists but belongs to another user
    // (or is protected) - treating that as "gone" would let a lock recovery
    // reclaim a lock a living process still holds. Only a pid that does not
    // exist is an answer; every other error stays on the safe side.
    return ::GetLastError() != ERROR_INVALID_PARAMETER;
  }
  DWORD exitCode = 0;
  const bool alive = ::GetExitCodeProcess(handle, &exitCode) != 0 && exitCode == STILL_ACTIVE;
  ::CloseHandle(handle);
  return alive;
#else
  if (::kill(static_cast<pid_t>(processId), 0) == 0) { return true; }
  return errno == EPERM;
#endif
}

std::int64_t processStartTicksOf(ProcessId processId) noexcept
{
  if (processId <= 0) { return 0; }
#if defined(_WIN32)
  const HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processId));
  if (handle == nullptr) { return 0; }
  FILETIME creation{};
  FILETIME exitTime{};
  FILETIME kernelTime{};
  FILETIME userTime{};
  std::int64_t ticks = 0;
  if (::GetProcessTimes(handle, &creation, &exitTime, &kernelTime, &userTime) != 0) {
    ULARGE_INTEGER value{};
    value.LowPart = creation.dwLowDateTime;
    value.HighPart = creation.dwHighDateTime;
    ticks = static_cast<std::int64_t>(value.QuadPart);
  }
  ::CloseHandle(handle);
  return ticks;
#else
  // /proc/<pid>/stat: the comm field (2) is parenthesised and may contain
  // spaces and parentheses, so the parse starts after the *last* ')'. The
  // remaining fields start at field 3, which makes starttime (field 22) the
  // token with index 19.
  std::ifstream stat("/proc/" + std::to_string(processId) + "/stat");
  if (!stat.is_open()) { return 0; }
  std::string line;
  std::getline(stat, line);
  const std::size_t commEnd = line.rfind(')');
  if (commEnd == std::string::npos || commEnd + 2 >= line.size()) { return 0; }
  const std::vector<std::string> rest = splitOnSpaces(line.substr(commEnd + 2));
  constexpr std::size_t kStartTimeIndex = 22 - 3;
  if (rest.size() <= kStartTimeIndex) { return 0; }
  try {
    return std::stoll(rest[kStartTimeIndex]);
  } catch (const std::exception &) {
    return 0;
  }
#endif
}

bool processIdentityIsAlive(const ProcessIdentity &identity) noexcept
{
  if (!processIsAlive(identity.processId)) { return false; }
  if (identity.startTicks == 0) { return true; }
  const std::int64_t current = processStartTicksOf(identity.processId);
  // A recycled pid belongs to a different process, which started later - that
  // one is not the holder, so the recorded identity is gone. An unavailable
  // start time (0) is treated as "still the same", because reclaiming a lock
  // that a living process holds would be the far worse mistake.
  return current == 0 || current == identity.startTicks;
}
}// namespace lintel
