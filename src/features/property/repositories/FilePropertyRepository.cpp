#include "lintel/features/property/repositories/FilePropertyRepository.h"

#include <utility>

#include "lintel/features/property/configuration/PropertyComponent.h"
#include "lintel/features/property/configuration/PropertyEntry.h"

/**
 * Deliberate no-op. This repository is immutable: PropertyRepositoryComponent::parse
 * rejects a FILE_REPOSITORY with mutable="true" or shadow="true" with a
 * ConfigurationException. Thus isMutable() is permanently false and
 * PropertyService never calls save() via filterMutableRepositories()/filterShadowRepositories()
 * nor deleteOf() in onAwake() (explicit isMutable() check).
 * The method is kept as the implementation of the pure interface
 * PropertyRepository::save(). Writing the file is documented as the goal in
 * FilePropertyRepository.h and not implemented yet.
 */
void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

/**
 * Deliberate no-op, rationale see FilePropertyRepository::save().
 * @param saveProperties properties that cannot be written
 */
void FilePropertyRepository::save(
        const std::vector<std::shared_ptr<PropertyBase>> &saveProperties) {}

/**
 * Returns the properties of this process read from the XML configuration.
 * @return properties of the process from m_properties
 */
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
    for (auto &&entry: entries) {
        std::shared_ptr<PropertyBase> property =
                std::static_pointer_cast<PropertyEntry>(entry)->getProperty();
        if (property->getProcessName() != processName->getProcessName()) {
            continue;
        }
        m_properties.push_back(property);
    }
}
