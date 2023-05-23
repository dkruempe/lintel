#include "base_library/core/utils/MemorySize.h"

#include <fmt/format.h>

#include "base_library/core/utils/StringUtils.h"

std::size_t MemorySize::deserialize(const std::string &size) {
    bool isByte = StringUtils::endsWith(size, "B");
    bool isKiloByte = StringUtils::endsWith(size, "kB");
    bool isMegaByte = StringUtils::endsWith(size, "MB");
    bool isGigaByte = StringUtils::endsWith(size, "GB");
    bool isTeraByte = StringUtils::endsWith(size, "TB");
    bool isPetaByte = StringUtils::endsWith(size, "PB");
    if (isKiloByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000;
    }
    if (isMegaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000;
    }
    if (isGigaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000;
    }
    if (isTeraByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000000;
    }
    if (isPetaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000000000;
    }
    if (isByte) {
        return std::stoul(size.substr(0, size.length() - 1));
    }
    return std::stoul(size);
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
