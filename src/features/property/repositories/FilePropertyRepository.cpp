#include "base_library/features/property/repositories/FilePropertyRepository.h"

#include <utility>

#include "base_library/features/property/configuration/PropertyComponent.h"
#include "base_library/features/property/configuration/PropertyEntry.h"

/**
 * Bewusstes No-Op. Dieses Repository ist unveränderlich: PropertyRepositoryComponent::parse
 * lehnt eine FILE_REPOSITORY mit mutable="true" oder shadow="true" per
 * ConfigurationException ab. Damit ist isMutable() dauerhaft false und
 * PropertyService ruft save() über filterMutableRepositories()/filterShadowRepositories()
 * sowie deleteOf() in onAwake() (explizite isMutable()-Prüfung) nie auf.
 * Die Methode bleibt als Implementierung des reinen Interfaces
 * PropertyRepository::save() erhalten. Das Dateischreiben ist als Ziel in
 * FilePropertyRepository.h dokumentiert und noch nicht umgesetzt.
 */
void FilePropertyRepository::save(std::shared_ptr<PropertyBase> property) {}

/**
 * Bewusstes No-Op, Begründung siehe FilePropertyRepository::save().
 * @param saveProperties Properties, die nicht geschrieben werden können
 */
void FilePropertyRepository::save(
        const std::vector<std::shared_ptr<PropertyBase>> &saveProperties) {}

/**
 * Liefert die aus der XML-Konfiguration gelesenen Properties dieses Prozesses.
 * @return Properties des Prozesses aus m_properties
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
