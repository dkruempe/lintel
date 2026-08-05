#include "base_library/core/configuration/Component.h"


#include <limits>
#include <utility>
import base_library.core.utils;

namespace {
std::size_t parseSizeAndScale(const std::string &input, const std::string &prefix,
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

Component::Component(std::string configRoot)
        : m_configRoot(std::move(configRoot)) {}

const std::string &Component::getConfigRoot() const { return m_configRoot; }

std::size_t Component::convertToBytes(const std::string &size) {
    bool isByte = StringUtils::endsWith(size, "B");
    bool isKiloByte = StringUtils::endsWith(size, "kB");
    bool isMegaByte = StringUtils::endsWith(size, "MB");
    bool isGigaByte = StringUtils::endsWith(size, "GB");
    bool isTeraByte = StringUtils::endsWith(size, "TB");
    bool isPetaByte = StringUtils::endsWith(size, "PB");
    if (isKiloByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 2), 1000);
    } else if (isMegaByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 2), 1000000);
    } else if (isGigaByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 2), 1000000000);
    } else if (isTeraByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 2), 1000000000000);
    } else if (isPetaByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 2), 1000000000000000);
    } else if (isByte) {
        return parseSizeAndScale(size, size.substr(0, size.length() - 1), 1);
    } else {
        return parseSizeAndScale(size, size, 1);
    }
}
