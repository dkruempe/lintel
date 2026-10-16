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

}// namespace lintel

#endif// LINTEL_CORE_PLATFORM_PLATFORM_H
