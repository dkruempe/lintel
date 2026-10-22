#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/models/DataStorage.h"
#include "lintel/core/models/PropertyRepositoryType.h"
#include "lintel/features/property/configuration/PropertyRepositoryComponent.h"
#include "lintel/features/property/configuration/PropertyRepositoryEntry.h"

namespace lintel {
class PropertyBase;

/**
 * Filter for PropertyRepository::allOf; every field is a regular expression.
 * The four coordinates are same-typed and follow the process/property
 * hierarchy, so a named aggregate keeps them from being swapped at the call
 * site.
 */
struct PropertyFilter
{
  /** Process name pattern; pass ".*" to match every process */
  std::string m_processName{ ".*" };
  /** Class name pattern; pass ".*" to match every class */
  std::string m_className{ ".*" };
  /** Instance name pattern; pass ".*" to match every instance */
  std::string m_instanceName{ ".*" };
  /** Property name pattern; pass ".*" to match every property */
  std::string m_name{ ".*" };

  /** Equality so the filter can be matched by value (trompeloeil, tests). */
  friend bool operator==(const PropertyFilter &lhs, const PropertyFilter &rhs)
  {
    return lhs.m_processName == rhs.m_processName && lhs.m_className == rhs.m_className
           && lhs.m_instanceName == rhs.m_instanceName && lhs.m_name == rhs.m_name;
  }
};

/** Abstract base class for property repositories */
class PropertyRepository
{
private:
  PropertyRepositoryType m_type;
  std::shared_ptr<PropertyRepositoryEntry> m_propertyRepositoryEntry;

  /** Initialize the repository entry from configuration */
  std::shared_ptr<PropertyRepositoryEntry> init(const std::vector<std::shared_ptr<Entry>> &entries)
  {
    std::shared_ptr<PropertyRepositoryEntry> propertyRepositoryEntry;
    for (const auto &entry : entries) {
      std::shared_ptr<PropertyRepositoryEntry> tmp = std::static_pointer_cast<PropertyRepositoryEntry>(entry);
      if (tmp->getType() != m_type) { continue; }
      propertyRepositoryEntry = tmp;
    }
    return propertyRepositoryEntry;
  }

public:
  /** @param type repository type
   *  @param configuration configuration source */
  PropertyRepository(PropertyRepositoryType type, const std::shared_ptr<Configuration> &configuration)
    : m_type(std::move(type)),
      m_propertyRepositoryEntry(init(configuration->configurationOf<PropertyRepositoryComponent>()))
  {}

  virtual ~PropertyRepository() = default;

  /**
   * returns priority of repository
   */
  PropertyRepositoryType getType() const { return m_type; }

  /** @return true if this is a shadow repository */
  bool isShadow() const
  {
    if (m_propertyRepositoryEntry == nullptr) { return false; }
    return m_propertyRepositoryEntry->isShadow();
  }

  /** @return true if this repository is enabled in configuration */
  bool isEnabled() const { return m_propertyRepositoryEntry != nullptr; }

  /** @return data storage info */
  virtual DataStorage getDataStorage() = 0;

  /**
   * returns if save operations are supported or not
   */
  bool isMutable() const
  {
    if (m_propertyRepositoryEntry == nullptr) { return false; }
    return m_propertyRepositoryEntry->isMutable();
  }

  /**
   * save properties to repository
   */
  virtual void save(const std::vector<std::shared_ptr<PropertyBase>> &properties) = 0;

  /**
   * save property to repository
   */
  virtual void save(std::shared_ptr<PropertyBase> property) = 0;

  /**
   * deletes properties in the mentioned repository
   *
   * No-op by default: a read-only repository has nothing to delete from.
   * `PropertyService` only reaches this method behind an `isMutable()`
   * check, so the default is unreachable for `FilePropertyRepository`,
   * which `PropertyRepositoryComponent::parse` rejects with a
   * ConfigurationException when mutable="true" is configured.
   */
  virtual void deleteOf(const std::vector<std::shared_ptr<PropertyBase>> &properties) {}

  /**
   * load all properties from repository and return result
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;

  /**
   * Query properties with optional filters
   *
   * Returns an empty vector by default: an implementation without query
   * support reports "no overrides", which `PropertyService` reads as
   * "keep the configured default". The default is only reached for
   * `FilePropertyRepository`, whose values come from the XML configuration
   * and are never queried at runtime.
   * @param filter regexes for process, class, instance and property name
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> allOf(const PropertyFilter &filter = {}) { return {}; }
};

}// namespace lintel
#endif// LOGGING_PROPERTYREPOSITORY_H
