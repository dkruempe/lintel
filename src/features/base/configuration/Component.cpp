#include "base_library/features/base/configuration/Component.h"

#include <base_library/core/utils/MemorySize.h>

#include <base_library/core/utils/StringUtils.h>

#include <limits>
#include <utility>

Component::Component(std::string configRoot)
        : m_configRoot(std::move(configRoot)) {}

const std::string &Component::getConfigRoot() const { return m_configRoot; }

std::size_t Component::convertToBytes(const std::string &size) {
    return MemorySize::deserialize(size);
}
