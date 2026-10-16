#ifndef LINTEL_CORE_UTILS_DATETIMEFORMAT_H
#define LINTEL_CORE_UTILS_DATETIMEFORMAT_H

#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

#include "lintel/core/platform/Platform.h"

namespace lintel {
/**
 * Portable, locale-independent timestamp formatting/parsing.
 *
 * The library used to rely on the C++20 chrono integrations of &lt;format&gt;
 * (`std::format("{:%Y-%m-%d %H:%M:%S%Ez}", sys_time<microseconds>)`) and of the
 * stream extraction operator (`std::chrono::parse(...)`). Both landed in libc++
 * only recently (chrono formatting in LLVM 21) and are still incomplete in
 * AppleClang's libc++, so they are not portable to macOS. libstdc++ and MSVC
 * support them, but with platform-specific corner cases (e.g. the decimal
 * separator of the active locale). These helpers replace both with a small,
 * self-contained implementation that produces the same wire format on every
 * platform and target:
 *
 *   "2021-09-08 17:04:49.580723+02:00"
 *
 * i.e. local time with microsecond resolution and a "+HH:MM" UTC offset. The
 * offset variants "±HH", "±HHMM" and "±HH:MM" are all accepted when parsing,
 * matching what PostgreSQL/SQLite store in the seed data.
 */
namespace datetime {

  /** Timestamp type used across the persistence layer. */
  using SystemTime = std::chrono::sys_time<std::chrono::microseconds>;

  /**
   * timegm() is POSIX, _mkgmtime() is MSVC; both interpret a std::tm as UTC.
   * @param tm broken-down UTC time (modified in place)
   * @return the corresponding time_t
   */
  inline std::time_t timegmPortable(std::tm *tm) noexcept
  {
#if defined(_WIN32)
    return ::_mkgmtime(tm);
#else
    return ::timegm(tm);
#endif
  }

  /**
   * @param time  a point in time
   * @param local its local-time representation (from lintel::localTime)
   * @return the local offset from UTC in seconds (east positive)
   */
  inline long localOffsetSeconds(std::time_t time, const std::tm &local) noexcept
  {
    // offset = local_wall_clock - utc_wall_clock = mktime(local) - timegm(local)
    // and mktime(local) == time, so the round trip through timegm() is enough.
    std::tm asUtc = local;
    return static_cast<long>(time - timegmPortable(&asUtc));
  }

  /** @param offsetSeconds offset from UTC in seconds @return true when east of UTC */
  inline bool isEastOfUtc(long offsetSeconds) noexcept { return offsetSeconds >= 0; }

  /** @param offsetSeconds offset from UTC in seconds @return offset formatted as "+HH:MM"/"-HH:MM" */
  inline std::string formatOffset(long offsetSeconds)
  {
    const long absolute = offsetSeconds < 0 ? -offsetSeconds : offsetSeconds;
    char buffer[16];
    std::snprintf(buffer,
      sizeof(buffer),
      "%c%02ld:%02ld",
      isEastOfUtc(offsetSeconds) ? '+' : '-',
      absolute / 3600,
      (absolute % 3600) / 60);
    return buffer;
  }

  /**
   * Format a timestamp the way the persistence layer stores it: local time,
   * microsecond resolution, "+HH:MM" offset.
   * @param value the timestamp (microsecond resolution)
   * @param dateSeparator separator used between the date components ('-' in the
   *        database, '.' in the human-readable user dump)
   * @return the formatted timestamp, or an empty string when the time cannot be converted
   */
  inline std::string formatSystemTime(const SystemTime &value, char dateSeparator = '-')
  {
    const auto wholeSeconds = std::chrono::floor<std::chrono::seconds>(value);
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(value - wholeSeconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(wholeSeconds));
    std::tm local{};
    if (!lintel::localTime(time, local)) { return {}; }
    const long offset = localOffsetSeconds(time, local);

    char buffer[64];
    const int written = std::snprintf(buffer,
      sizeof(buffer),
      "%04d%c%02d%c%02d %02d:%02d:%02d.%06ld",
      local.tm_year + 1900,
      dateSeparator,
      local.tm_mon + 1,
      dateSeparator,
      local.tm_mday,
      local.tm_hour,
      local.tm_min,
      local.tm_sec,
      static_cast<long>(micros));
    if (written < 0) { return {}; }
    return std::string(buffer, static_cast<std::size_t>(written)) + formatOffset(offset);
  }

  /**
   * Parse a timestamp produced by formatSystemTime (or by PostgreSQL/SQLite).
   * Accepts "YYYY-MM-DD HH:MM:SS[.ffffff]" followed by a required UTC offset in
   * one of the forms "Z", "+HH", "-HH", "+HHMM", "+HH:MM".
   * @param value the string to parse
   * @return the parsed timestamp, or a default-constructed (epoch) value when the
   *         string cannot be parsed - mirroring std::chrono::parse's behaviour
   */
  inline SystemTime parseSystemTime(const std::string &value)
  {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int consumed = 0;
    if (std::sscanf(value.c_str(), "%d-%d-%d %d:%d:%d%n", &year, &month, &day, &hour, &minute, &second, &consumed)
        != 6) {
      return SystemTime{};
    }

    std::size_t index = static_cast<std::size_t>(consumed);
    long micros = 0;
    if (index < value.size() && value[index] == '.') {
      ++index;
      long scale = 100000;
      while (index < value.size() && std::isdigit(static_cast<unsigned char>(value[index])) != 0) {
        if (scale > 0) {
          micros += static_cast<long>(value[index] - '0') * scale;
          scale /= 10;
        }
        ++index;
      }
    }

    // The offset is mandatory (PostgreSQL/SQLite always write one). "Z" means UTC.
    long offsetSeconds = 0;
    if (index < value.size() && (value[index] == 'Z' || value[index] == 'z')) {
      ++index;
    } else if (index < value.size() && (value[index] == '+' || value[index] == '-')) {
      const bool negative = value[index] == '-';
      ++index;
      const auto readNumber = [&value, &index](int maxDigits) -> int {
        int result = 0;
        int digits = 0;
        while (
          index < value.size() && digits < maxDigits && std::isdigit(static_cast<unsigned char>(value[index])) != 0) {
          result = result * 10 + (value[index] - '0');
          ++index;
          ++digits;
        }
        return result;
      };
      const int offsetHours = readNumber(2);
      if (index < value.size() && value[index] == ':') { ++index; }
      const int offsetMinutes = readNumber(2);
      offsetSeconds = static_cast<long>(offsetHours) * 3600 + static_cast<long>(offsetMinutes) * 60;
      if (negative) { offsetSeconds = -offsetSeconds; }
    } else {
      return SystemTime{};
    }

    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = 0;
    const std::time_t utcSeconds = timegmPortable(&tm) - offsetSeconds;
    return SystemTime{ std::chrono::seconds{ utcSeconds } } + std::chrono::microseconds{ micros };
  }

  /**
   * Format a UTC timestamp as ISO-8601 with a second resolution and a "Z" suffix
   * (the "{:%FT%TZ}" form used by the CLI prompt).
   * @param value the timestamp
   * @return e.g. "2026-10-16T07:49:08Z", or an empty string on failure
   */
  inline std::string formatUtcIsoSeconds(const std::chrono::sys_seconds &value)
  {
    const std::time_t time =
      std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(value));
    std::tm utc{};
#if defined(_WIN32)
    if (::gmtime_s(&utc, &time) != 0) { return {}; }
#else
    if (::gmtime_r(&time, &utc) == nullptr) { return {}; }
#endif
    char buffer[32];
    const int written = std::snprintf(buffer,
      sizeof(buffer),
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      utc.tm_year + 1900,
      utc.tm_mon + 1,
      utc.tm_mday,
      utc.tm_hour,
      utc.tm_min,
      utc.tm_sec);
    if (written < 0) { return {}; }
    return std::string(buffer, static_cast<std::size_t>(written));
  }

}// namespace datetime
}// namespace lintel

#endif// LINTEL_CORE_UTILS_DATETIMEFORMAT_H
