#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>
#include <vector>

#include "base_library/core/configuration/Configuration.h"
#include "base_library/features/property/configuration/PropertyRepositoryComponent.h"
#include "base_library/features/property/configuration/PropertyRepositoryEntry.h"
#include "base_library/core/property/DataStorage.h"
#include "base_library/core/property/PropertyRepositoryType.h"

class PropertyBase;

/** Abstract base class for property repositories */
class PropertyRepository {
private:
    PropertyRepositoryType m_type;
    std::shared_ptr<PropertyRepositoryEntry> m_propertyRepositoryEntry;

    /** Initialize the repository entry from configuration */
    std::shared_ptr<PropertyRepositoryEntry> init(
            const std::vector<std::shared_ptr<Entry>> &entries) {
        std::shared_ptr<PropertyRepositoryEntry> propertyRepositoryEntry;
        for (const auto &entry: entries) {
            std::shared_ptr<PropertyRepositoryEntry> tmp =
                    std::static_pointer_cast<PropertyRepositoryEntry>(entry);
            if (tmp->getType() != m_type) {
                continue;
            }
            propertyRepositoryEntry = tmp;
        }
        return propertyRepositoryEntry;
    }

public:
    /** @param type repository type
     *  @param configuration configuration source */
    PropertyRepository(PropertyRepositoryType type,
                       const std::shared_ptr<Configuration> &configuration)
            : m_type(std::move(type)),
              m_propertyRepositoryEntry(init(
                      configuration->configurationOf<PropertyRepositoryComponent>())) {}

    virtual ~PropertyRepository() = default;

    /**
     * returns priority of repository
     */
    PropertyRepositoryType getType() const { return m_type; }

    /** @return true if this is a shadow repository */
    bool isShadow() const {
        if (m_propertyRepositoryEntry == nullptr) {
            return false;
        }
        return m_propertyRepositoryEntry->isShadow();
    }

    /** @return true if this repository is enabled in configuration */
    bool isEnabled() const { return m_propertyRepositoryEntry != nullptr; }

    /** @return data storage info */
    virtual DataStorage getDataStorage() = 0;

    /**
     * returns if save operations are supported or not
     */
    bool isMutable() const {
        if (m_propertyRepositoryEntry == nullptr) {
            return false;
        }
        return m_propertyRepositoryEntry->isMutable();
    }

    /**
     * save properties to repository
     */
    virtual void save(
            const std::vector<std::shared_ptr<PropertyBase>> &properties) = 0;

    /**
     * save property to repository
     */
    virtual void save(std::shared_ptr<PropertyBase> property) = 0;

    /**
     * deletes properties in the mentioned repository
     */
    virtual void deleteOf(
            const std::vector<std::shared_ptr<PropertyBase>> &properties) {}

    /**
     * load all properties from repository and return result
     */
    virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;

    /**
     * Query properties with optional filters
     * @param processName regex for process name
     * @param className regex for class name
     * @param instanceName regex for instance name
     * @param name regex for property name
     */
    virtual std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName = ".*",
            const std::string &className = ".*",
            const std::string &instanceName = ".*", const std::string &name = ".*") {
        return {};
    }
};

#endif  // LOGGING_PROPERTYREPOSITORY_H
