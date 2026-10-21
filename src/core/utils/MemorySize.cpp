#include "lintel/core/utils/MemorySize.h"

#include <array>
#include <charconv>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include "lintel/core/utils/StringUtils.h"

namespace lintel {
namespace {
  constexpr std::size_t kBytesPerKilo = 1000;
  constexpr std::size_t kBytesPerMega = 1000000;
  constexpr std::size_t kBytesPerGiga = 1000000000;
  constexpr std::size_t kBytesPerTera = 1000000000000;
  constexpr std::size_t kBytesPerPeta = 1000000000000000;

  /** Unit suffix of a memory size and the number of bytes it stands for. */
  struct MemoryUnit
  {
    std::string_view m_suffix;
    std::size_t m_bytesPerUnit;
  };

  /** Longer suffixes first, so that "1kB" matches kB instead of the trailing B. */
  constexpr std::array<MemoryUnit, 6> kUnits{ {
    { "kB", kBytesPerKilo },
    { "MB", kBytesPerMega },
    { "GB", kBytesPerGiga },
    { "TB", kBytesPerTera },
    { "PB", kBytesPerPeta },
    { "B", 1 },
  } };

  /** Parses the number in front of the unit suffix and scales it to bytes. */
  std::size_t parseAndScale(std::string_view input, const MemoryUnit &unit)
  {
    std::string_view const number = input.substr(0, input.size() - unit.m_suffix.size());
    unsigned long value = 0;
    const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), value);
    if (error == std::errc::result_out_of_range) {
      throw std::invalid_argument(std::format("memory size overflow: '{}'", input));
    }
    // Reject unparsable input as well as a parsed number followed by trailing garbage.
    if (error != std::errc{} || end != number.data() + number.size()) {
      throw std::invalid_argument(std::format("invalid memory size: '{}'", input));
    }
    if (value > (std::numeric_limits<std::size_t>::max() / unit.m_bytesPerUnit)) {
      throw std::invalid_argument(std::format("memory size overflow: '{}'", input));
    }
    return static_cast<std::size_t>(value) * unit.m_bytesPerUnit;
  }
}// namespace

std::size_t MemorySize::deserialize(const std::string &size)
{
  for (const auto &unit : kUnits) {
    if (StringUtils::endsWith(size, unit.m_suffix)) { return parseAndScale(size, unit); }
  }
  return parseAndScale(size, MemoryUnit{ "", 1 });
}

std::string MemorySize::serialize(std::size_t byte)
{
  double petaByte = static_cast<double>(byte) / kBytesPerPeta;
  double teraByte = static_cast<double>(byte) / kBytesPerTera;
  double gigaByte = static_cast<double>(byte) / kBytesPerGiga;
  double megaByte = static_cast<double>(byte) / kBytesPerMega;
  double kiloByte = static_cast<double>(byte) / kBytesPerKilo;
  if (petaByte > 1) { return toString(petaByte) + "PB"; }
  if (teraByte > 1) { return toString(teraByte) + "TB"; }
  if (gigaByte > 1) { return toString(gigaByte) + "GB"; }
  if (megaByte > 1) { return toString(megaByte) + "MB"; }
  if (kiloByte > 1) { return toString(kiloByte) + "KB"; }
  return std::to_string(byte) + "B";
}

std::string MemorySize::toString(double value)
{
  std::string temp = std::format("{}", value);
  return temp;
}
}// namespace lintel
