#include "base_library/features/cli/models/CommandLineComponent.h"

CommandLineComponent::CommandLineComponent(std::string_view name,
                                           std::string_view alias)
        : m_name(name), m_alias(alias) {}

std::string_view CommandLineComponent::getName() { return m_name; }

std::string_view CommandLineComponent::getAlias() { return m_alias; }