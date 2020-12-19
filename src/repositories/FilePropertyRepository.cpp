#include "base_library/repositories/FilePropertyRepository.h"

#include <utility>

#include "base_library/config.h"

void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {
  std::vector<std::shared_ptr<PropertyBase>> vector = {property};
  save(vector);
}

void FilePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {
  FileService file(configurationPath);
  std::string newContent = configSerializationStrategy->serialize(properties);
  file.writeToFile(newContent, true);
}

std::vector<std::shared_ptr<PropertyBase>> FilePropertyRepository::awake() {
  FileService file(configurationPath);
  if (!file.exists()) {
    // throw ConfigurationNotFoundException
    return std::vector<std::shared_ptr<PropertyBase>>();
  }
  content = file.readFile();

  return configSerializationStrategy->deserialize(content);
}

PropertyRepositoryType FilePropertyRepository::getType() {
  return PropertyRepositoryType::FILE_REPOSITORY;
}

bool FilePropertyRepository::isMutable() { return true; }
FilePropertyRepository::FilePropertyRepository(
    std::shared_ptr<ConfigSerializationStrategy> configSerializationStrategy)
    : configurationPath(std::string(CONFIG_DIRECTORY) + std::filesystem::path::preferred_separator + "bootstrap.xml"),
      configSerializationStrategy(std::move(configSerializationStrategy)) {}
