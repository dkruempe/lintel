#include "base_library/features/property/repositories/FilePropertyRepository.h"

#include <utility>

#include "base_library/features/property/configuration/PropertyComponent.h"
#include "base_library/features/property/configuration/PropertyEntry.h"

void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

void FilePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {}

std::vector<std::shared_ptr<PropertyBase>> FilePropertyRepository::awake() {
  return properties;
}

PropertyRepositoryType FilePropertyRepository::getType() {
  return PropertyRepositoryType::FILE_REPOSITORY;
}

DataStorage FilePropertyRepository::getDataStorage() {
  // unused only needed for mutable repositories
  return DataStorage(getType(), "");
}

bool FilePropertyRepository::isMutable() { return false; }

FilePropertyRepository::FilePropertyRepository(
    std::shared_ptr<Configuration> configuration)
    : configuration(std::move(configuration)) {
  auto entries = configuration->configurationOf<PropertyComponent>();
  properties.reserve(entries.size());
  for (auto &&entry : entries) {
    properties.push_back(
        std::static_pointer_cast<PropertyEntry>(entry)->getProperty());
  }
}
