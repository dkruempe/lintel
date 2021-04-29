#include "base_library/features/base/configuration/Component.h"

#include <utility>

Component::Component(std::string configRoot)
    : m_configRoot(std::move(configRoot)) {}
const std::string &Component::getConfigRoot() const { return m_configRoot; }
