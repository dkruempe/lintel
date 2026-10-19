#ifndef LINTEL_CORE_PLATFORM_PLATFORM_H
#define LINTEL_CORE_PLATFORM_PLATFORM_H

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>

#if defined(_WIN32)
// Keep the transitive Windows headers from defining the min/max macros: the
// library uses std::min/std::max throughout, and a consumer that includes
// <Windows.h> before a lintel header would otherwise see them expand.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <process.h>// _getpid
#else
#include <sys/types.h>// pid_t
#include <unistd.h>// getpid
#endif

namespace lintel {
/**
 * Portable process id type.
 *
 * POSIX uses pid_t, Windows uses DWORD. A fixed 64-bit type keeps the value
 * lossless on both sides and lets the DTOs (JSON "process_id") stay identical
 * across platforms without dragging <Windows.h> into public headers.
 */
using ProcessId = std::int64_t;

/** @return the id of the calling process */
inline ProcessId currentProcessId() noexcept
{
#if defined(_WIN32)
  return static_cast<ProcessId>(::_getpid());
#else
  return static_cast<ProcessId>(::getpid());
#endif
}

/**
 * Thread-safe local-time conversion.
 *
 * Wraps localtime_r() (POSIX) resp. localtime_s() (MSVC, argument order
 * reversed) so callers do not have to branch.
 * @param time the time to convert
 * @param out  receives the broken-down local time
 * @return true on success
 */
inline bool localTime(std::time_t time, std::tm &out) noexcept
{
#if defined(_WIN32)
  return ::localtime_s(&out, &time) == 0;
#else
  return ::localtime_r(&time, &out) != nullptr;
#endif
}

/** @return the user's home directory, or an empty string when it cannot be resolved */
inline std::string homeDirectory()
{
#if defined(_WIN32)
  // Windows has no HOME; USERPROFILE is the canonical variable, HOMEDRIVE +
  // HOMEPATH is the legacy fallback for stripped-down environments.
  if (const char *profile = std::getenv("USERPROFILE"); profile != nullptr && *profile != '\0') { return profile; }
  const char *drive = std::getenv("HOMEDRIVE");
  const char *path = std::getenv("HOMEPATH");
  if (drive != nullptr && path != nullptr) { return std::string(drive) + path; }
  return {};
#else
  const char *home = std::getenv("HOME");
  return home != nullptr ? home : std::string{};
#endif
}

/** @return the character that separates entries in the PATH environment variable */
inline constexpr char pathListSeparator() noexcept
{
#if defined(_WIN32)
  return ';';
#else
  return ':';
#endif
}

/**
 * Check whether a process id still refers to a living process.
 *
 * The crash recovery of the shared memory locks needs to tell "the holder of
 * this lock went away" from "the holder is alive but slow"; only the first one
 * may be reclaimed. The implementation lives in Platform.cpp because it needs
 * <Windows.h>, which this header deliberately avoids pulling into public ones.
 *
 * @param processId the process to check, 0 or negative is never alive
 * @return true if a process with that id exists
 */
bool processIsAlive(ProcessId processId) noexcept;

/**
 * Platform ticks at which a process was started.
 *
 * Process ids are recycled, so a recorded id alone cannot answer "is this still
 * the process that held the lock?". Comparing the start time as well turns that
 * into a usable identity. The unit is platform specific (clock ticks since boot
 * on POSIX, 100ns since 1601 on Windows) - only values from the same machine
 * are comparable, which is exactly the use case.
 *
 * @param processId the process to inspect
 * @return the start ticks, or 0 when the process does not exist or the value is
 *         unavailable (a caller must then treat the identity as "unknown")
 */
std::int64_t processStartTicksOf(ProcessId processId) noexcept;

/**
 * Identity of one process: its id and the platform ticks it started at.
 *
 * A process id is recycled by the OS, so the pair is what identifies "the
 * process that recorded itself" over time.
 */
struct ProcessIdentity
{
  ProcessId processId = 0;
  std::int64_t startTicks = 0;
};

/**
 * Check whether a recorded process identity still denotes a living process.
 *
 * A recorded start time of 0 means "unknown" and falls back to the process id
 * alone, so a caller that cannot determine the ticks never turns a live holder
 * into a dead one - the failure direction of a lock recovery must always be
 * "leave the lock alone".
 *
 * @param identity the recorded process id and start ticks
 * @return true if the recorded process is still alive
 */
bool processIdentityIsAlive(const ProcessIdentity &identity) noexcept;

}// namespace lintel

#endif// LINTEL_CORE_PLATFORM_PLATFORM_H
