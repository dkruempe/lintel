#include "lintel/core/platform/Platform.h"

#include <cerrno>

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
  if (handle == nullptr) { return false; }
  DWORD exitCode = 0;
  const bool alive = ::GetExitCodeProcess(handle, &exitCode) != 0 && exitCode == STILL_ACTIVE;
  ::CloseHandle(handle);
  return alive;
#else
  if (::kill(static_cast<pid_t>(processId), 0) == 0) { return true; }
  return errno == EPERM;
#endif
}
}// namespace lintel
