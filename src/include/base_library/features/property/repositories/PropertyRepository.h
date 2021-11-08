#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/property/configuration/PropertyRepositoryComponent.h"
#include "base_library/features/property/configuration/PropertyRepositoryEntry.h"
#include "base_library/features/property/models/DataStorage.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

class PropertyBase;

class PropertyRepository {
 private:
  PropertyRepositoryType m_type;
  std::shared_ptr<PropertyRepositoryEntry> m_propertyRepositoryEntry;

  std::shared_ptr<PropertyRepositoryEntry> init(
      const std::vector<std::shared_ptr<Entry>> &entries) {
    std::shared_ptr<PropertyRepositoryEntry> propertyRepositoryEntry;
    for (const auto &entry : entries) {
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
  PropertyRepository(PropertyRepositoryType type,
                     const std::shared_ptr<Configuration> &configuration)
      : m_type(std::move(type)),
        m_propertyRepositoryEntry(init(
            configuration->configurationOf<PropertyRepositoryComponent>())) {}

  virtual ~PropertyRepository() = default;

  /**
   * returns priority of repository
   */
  PropertyRepositoryType getType() { return m_type; }

  bool isShadow() const {
    if (m_propertyRepositoryEntry == nullptr) {
      return false;
    }
    return m_propertyRepositoryEntry->isShadow();
  }

  bool isEnabled() const { return m_propertyRepositoryEntry != nullptr; }

  virtual DataStorage getDataStorage() = 0;

  /**
   * returns if save operations are supported or not
   */
  bool isMutable() {
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
   * load all properties from repository and return result
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;
};

#endif  // LOGGING_PROPERTYREPOSITORY_H
