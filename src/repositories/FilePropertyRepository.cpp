#include "base_library/repositories/FilePropertyRepository.h"

FilePropertyRepository::FilePropertyRepository(
    const std::filesystem::path &configurationPath,
    ConfigSerializationStrategy &configSerializationStrategy)
    : configurationPath(std::move(configurationPath)),
      configSerializationStrategy(configSerializationStrategy) {}

void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {
  std::vector<std::shared_ptr<PropertyBase>> vector = {property};
  save(vector);
}

void FilePropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {
  File file(configurationPath);
  std::string newContent = configSerializationStrategy.serialize(properties);
  file.writeToFile(newContent, true);
  return;
}

std::vector<std::shared_ptr<PropertyBase>> FilePropertyRepository::awake() {
  File file(configurationPath);
  if (!file.exists()) {
    // throw ConfigurationNotFoundException
    return std::vector<std::shared_ptr<PropertyBase>>();
  }
  content = file.readFile();

  return configSerializationStrategy.deserialize(content);
}