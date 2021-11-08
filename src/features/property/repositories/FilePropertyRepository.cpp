#include "base_library/features/property/repositories/FilePropertyRepository.h"

#include <utility>

#include "base_library/features/property/configuration/PropertyComponent.h"
#include "base_library/features/property/configuration/PropertyEntry.h"

void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

void FilePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &saveProperties) {}

std::vector<std::shared_ptr<PropertyBase>> FilePropertyRepository::awake() {
  return m_properties;
}

DataStorage FilePropertyRepository::getDataStorage() {
  // unused only needed for mutable repositories
  return DataStorage(getType(), "");
}

FilePropertyRepository::FilePropertyRepository(
    std::shared_ptr<Configuration> configuration,
    const std::shared_ptr<ProcessName> &processName)
    : PropertyRepository(PropertyRepositoryType::FILE_REPOSITORY,
                         configuration),
      m_configuration(std::move(configuration)) {
  auto entries = m_configuration->configurationOf<PropertyComponent>();
  m_properties.reserve(entries.size());
  for (auto &&entry : entries) {
    std::shared_ptr<PropertyBase> property =
        std::static_pointer_cast<PropertyEntry>(entry)->getProperty();
    if (property->getProcessName() != processName->getProcessName()) {
      continue;
    }
    m_properties.push_back(property);
  }
}
