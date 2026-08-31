#ifndef CPP_BASE_LIBRARY_PROCESSRESOURCEREADER_H
#define CPP_BASE_LIBRARY_PROCESSRESOURCEREADER_H

#include <boost/process/v1/child.hpp>

#include <chrono>
#include <cstdint>
#include <optional>

/**
 * Process resource snapshot read from the operating system (Linux /proc or
 * macOS libproc). Values are optional / deltas so that only successfully
 * read values are reported; missing values are never faked as zero.
 */
struct ProcessResourceData
{
  /** Wall-clock sample timestamp used to compute elapsed time for a CPU delta. */
  std::chrono::steady_clock::time_point sampleTime = std::chrono::steady_clock::now();
  /** Cumulative process CPU time in clock ticks at sampling time. */
  std::optional<uint64_t> cpuTicksTotal;
  /** Process uptime. */
  std::chrono::seconds uptime = std::chrono::seconds(0);
  /** CPU usage in percent over the sampling delta, if two samples are available. */
  std::optional<double> cpuPercent;
  /** Resident memory usage in bytes. */
  std::optional<uint64_t> memoryBytes;
  /** True if any field could be read successfully on this platform. */
  bool valid = false;
};

/**
 * Reads per-process resource usage. Supports Linux (/proc) and macOS (libproc).
 * On other platforms all values are reported as invalid (no resource data).
 * Pass the previously returned snapshot into the next call to compute a CPU
 * percentage delta.
 */
class ProcessResourceReader
{
public:
  ProcessResourceReader() = delete;

  /**
   * Read the current resource snapshot for the given process.
   * @param pid the operating system process id
   * @param previous a previously read snapshot used to compute the CPU delta
   * @return the resource snapshot (valid=false on unsupported platforms or unreadable pid)
   */
  static ProcessResourceData readOf(boost::process::v1::pid_t pid, const std::optional<ProcessResourceData> &previous);
};

#endif// CPP_BASE_LIBRARY_PROCESSRESOURCEREADER_H
