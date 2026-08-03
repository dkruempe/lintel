#include "base_library/core/utils/MemorySize.h"

#include <fmt/format.h>

#include <limits>

#include "base_library/core/utils/StringUtils.h"

namespace {
/** Parses a numeric prefix and validates it, optionally scaling it by a multiplier. */
std::size_t parseAndScale(const std::string &input, const std::string &prefix,
                          std::size_t multiplier) {
    std::size_t pos = 0;
    unsigned long value = 0;
    try {
        value = std::stoul(prefix, &pos);
    } catch (const std::exception &) {
        throw std::invalid_argument("invalid memory size: '" + input + "'");
    }
    if (pos != prefix.size()) {
        throw std::invalid_argument("invalid memory size: '" + input + "'");
    }
    if (value > (std::numeric_limits<std::size_t>::max() / multiplier)) {
        throw std::invalid_argument("memory size overflow: '" + input + "'");
    }
    return static_cast<std::size_t>(value) * multiplier;
}
}  // namespace

std::size_t MemorySize::deserialize(const std::string &size) {
    bool isByte = StringUtils::endsWith(size, "B");
    bool isKiloByte = StringUtils::endsWith(size, "kB");
    bool isMegaByte = StringUtils::endsWith(size, "MB");
    bool isGigaByte = StringUtils::endsWith(size, "GB");
    bool isTeraByte = StringUtils::endsWith(size, "TB");
    bool isPetaByte = StringUtils::endsWith(size, "PB");
    if (isKiloByte) {
        return parseAndScale(size, size.substr(0, size.length() - 2), 1000);
    }
    if (isMegaByte) {
        return parseAndScale(size, size.substr(0, size.length() - 2), 1000000);
    }
    if (isGigaByte) {
        return parseAndScale(size, size.substr(0, size.length() - 2), 1000000000);
    }
    if (isTeraByte) {
        return parseAndScale(size, size.substr(0, size.length() - 2), 1000000000000);
    }
    if (isPetaByte) {
        return parseAndScale(size, size.substr(0, size.length() - 2), 1000000000000000);
    }
    if (isByte) {
        return parseAndScale(size, size.substr(0, size.length() - 1), 1);
    }
    return parseAndScale(size, size, 1);
}

std::string MemorySize::serialize(std::size_t byte) {
    double petaByte = static_cast<double>(byte) / 1000000000000000.0;
    double teraByte = static_cast<double>(byte) / 1000000000000.0;
    double gigaByte = static_cast<double>(byte) / 1000000000.0;
    double megaByte = static_cast<double>(byte) / 1000000.0;
    double kiloByte = static_cast<double>(byte) / 1000.0;
    if (petaByte > 1) {
        return toString(petaByte) + "PB";
    }
    if (teraByte > 1) {
        return toString(teraByte) + "TB";
    }
    if (gigaByte > 1) {
        return toString(gigaByte) + "GB";
    }
    if (megaByte > 1) {
        return toString(megaByte) + "MB";
    }
    if (kiloByte > 1) {
        return toString(kiloByte) + "KB";
    }
    return std::to_string(byte) + "B";
}

std::string MemorySize::toString(double value) {
    std::string temp = fmt::format("{}", value);
    return temp;
}
