#include "base_library/features/cli/models/CommandLineComponent.h"

CommandLineComponent::CommandLineComponent(std::string_view name,
                                           std::string_view alias)
    : m_name(name), m_alias(alias) {}

std::string_view CommandLineComponent::getName() { return m_name; }

std::string_view CommandLineComponent::getAlias() { return m_alias; }

void CommandLineComponent::setClient(const std::shared_ptr<Client> &client) {
  m_client = client;
}

const std::shared_ptr<Client> &CommandLineComponent::getClient() {
  return m_client;
}