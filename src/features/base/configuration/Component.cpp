#include "base_library/features/base/configuration/Component.h"

#include <base_library/core/utils/StringUtils.h>

#include <utility>

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
        return std::stoul(size.substr(0, size.length() - 2)) * 1000;
    } else if (isMegaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000;
    } else if (isGigaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000;
    } else if (isTeraByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000000;
    } else if (isPetaByte) {
        return std::stoul(size.substr(0, size.length() - 2)) * 1000000000000000;
    } else if (isByte) {
        return std::stoul(size.substr(0, size.length() - 1));
    } else {
        return std::stoul(size);
    }
}
