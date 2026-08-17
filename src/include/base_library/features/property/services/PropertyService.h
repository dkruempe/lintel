#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/base/events/EventBus.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/property/events/PropertyChange.h"
#include "base_library/features/property/exceptions/PropertyNoRuntimeChangeSupported.h"
#include "base_library/features/property/exceptions/PropertyNotFoundException.h"
#include "base_library/features/property/models/Property.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

#define DEFINE_PROPERTY(name, type, defaultValue, description, runtime)     \
  std::shared_ptr<Property<type>> name =                                    \
      registerProperty<type>(std::string(#name), defaultValue, description, \
                             runtime, __FILE__, __LINE__)
#define LOAD_PROPERTIES()                       \
  if (propertyService != nullptr) {             \
    propertyService->getOrCreate(m_properties); \
  }

/** Central service for managing, querying and persisting properties */
class PropertyService : public PersistableBean {
private:
    std::vector<std::shared_ptr<PropertyRepository>> m_propertyRepositories;
    std::vector<std::shared_ptr<PropertyRepository>>
            m_mutablePropertyRepositories;
    std::vector<std::shared_ptr<PropertyRepository>> m_shadowPropertyRepositories;
    std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;
    // variables
    std::map<std::string, std::shared_ptr<PropertyBase>>
            m_properties;  // identifier (name_instanceName_processName), Property
    std::map<PropertyRepositoryType, std::shared_ptr<PropertyRepository>>
            m_typeRepositoryMap;
    std::shared_ptr<IEventBus> m_changeBus;
    std::string m_changeSubscriber;

    // functions
    /** Build a unique identifier string from property coordinates */
    static std::string createIdentifier(const std::string &name,
                                        const std::string &instanceName,
                                        const std::string &className,
                                        const std::string &processName);

    /** Initialize the property map from repository data */
    static std::map<std::string, std::shared_ptr<PropertyBase>> init(
            const std::vector<std::shared_ptr<PropertyRepository>> &repoProperties,
            std::map<PropertyRepositoryType,
                    std::vector<std::shared_ptr<PropertyBase>>> &propertiesMap);

    /** Filter to only enabled repositories */
    static std::vector<std::shared_ptr<PropertyRepository>>
    filterEnabledRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepositories);

    /** Filter to only mutable repositories */
    static std::vector<std::shared_ptr<PropertyRepository>>
    filterMutableRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepository);

    /** Filter to only shadow repositories */
    static std::vector<std::shared_ptr<PropertyRepository>>
    filterShadowRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepository);

    /** Match properties from vector against existing map */
    void getOrCreate(
            const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec,
            std::map<PropertyRepositoryType,
                    std::vector<std::shared_ptr<PropertyBase>>> &map);

    /** Publish a change notification on the configured bus.
     * @param property the changed property */
    void publishChange(const std::shared_ptr<PropertyBase> &property);

    /** Apply a change notification to a local property.
     * @param change the received change notification
     * @return true if a local property was updated */
    bool applyChange(const PropertyChange &change);

    /** Resolve the full value of a truncated change notification by reading it
     * from a locked source (the shared memory property repository).
     * @param change the truncated change notification
     * @return the full value, or std::nullopt if it could not be resolved */
    std::optional<std::string> resolveChangeValue(const PropertyChange &change);

public:
    /** Event bus topic carrying property change notifications. */
    static constexpr std::string_view PROPERTY_CHANGES_TOPIC =
            "property_changes";
    /** Event type of property change notifications. */
    static constexpr std::string_view PROPERTY_CHANGES_TYPE = "property_change";
    /** @param propertyRepositories list of all property repositories
     *  @param abstractServices list of services to notify on property changes */
    PropertyService(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepositories,
            std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices);

    virtual ~PropertyService() = default;

    void onAwake() override;

    /** Match properties from vector against existing map */
    void getOrCreate(
            const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec) {
        std::map<PropertyRepositoryType, std::vector<std::shared_ptr<PropertyBase>>>
                map;
        return getOrCreate(propertiesVec, map);
    }

    /**
     * Get or create a typed property
     * @tparam T property value type
     * @param name property name
     * @param instanceName instance name
     * @param className class name
     * @param processName process name
     * @param description property description
     * @param runtimeChange whether runtime changes are supported
     * @param defaultValue default value if property does not exist
     * @return the property
     */
    template<class T>
    std::shared_ptr<Property<T>> getOrCreate(const std::string &name,
                                             const std::string &instanceName,
                                             const std::string &className,
                                             const std::string &processName,
                                             const std::string &description,
                                             bool runtimeChange,
                                             const T &defaultValue = T());

    /**
     * Get a property by its coordinates
     * @throws PropertyNotFoundException if not found
     */
    std::shared_ptr<PropertyBase> &get(const std::string &name,
                                       const std::string &instanceName,
                                       const std::string &className,
                                       const std::string &processName);

    /** @return all properties */
    std::vector<std::shared_ptr<PropertyBase>> allOf();

    /** @return properties matching the given process name */
    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName);

    /** @return properties matching process name and class name */
    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName, const std::string &className);

    /** @return properties matching process, class, and instance name */
    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName, const std::string &className,
            const std::string &instanceName);

    /**
     * Change the value of a property at runtime
     * @throws PropertyNoRuntimeChangeSupported if property does not allow it
     */
    template<class T>
    void changeValueOf(const std::shared_ptr<PropertyBase> &property,
                       const T &value);

    /** Change the value of a property from string representation */
    void changeStringValueOf(const std::shared_ptr<PropertyBase> &property,
                             const std::string &value);

    /** Connect the change notification bus and subscribe for property changes.
     * @param bus the event bus view used for notifications
     * @param subscriberName a name unique to this process */
    void setPropertyChangeBus(const std::shared_ptr<IEventBus> &bus,
                              const std::string &subscriberName);

    /** Drain pending change notifications and apply them to local properties.
     * @return the number of applied changes */
    std::size_t applyChangeNotifications();

    /** Build repository type-to-instance map */
    std::map<PropertyRepositoryType, std::shared_ptr<PropertyRepository>>
    buildMap(const std::vector<std::shared_ptr<PropertyRepository>> &vector);
};

// template functions implementations
template<class T>
std::shared_ptr<Property<T>> PropertyService::getOrCreate(
        const std::string &name, const std::string &instanceName,
        const std::string &className, const std::string &processName,
        const std::string &description, bool runtimeChange,
        const T &defaultValue /* T() default Value */) {
    auto id = createIdentifier(name, instanceName, className, processName);
    try {
        auto propertyBase = get(name, instanceName, className, processName);
        return std::static_pointer_cast<Property<T>>(propertyBase);
    } catch (PropertyNotFoundException &exception) {
        auto property = std::make_shared<Property<T>>(name, instanceName, className,
                                                       processName, defaultValue,
                                                       description, runtimeChange);
        m_properties.insert({property->getIdentifier(), property});
        if (m_mutablePropertyRepositories.empty()) {
            LOG_ERROR("no mutable property repositories available");
            return property;
        }
        for (const auto &item: m_mutablePropertyRepositories) {
            item->save(property);
        }
        return property;
    }
}

template<class T>
void PropertyService::changeValueOf(
        const std::shared_ptr<PropertyBase> &property, const T &value) {
    std::shared_ptr<PropertyBase> propertyBase =
            get(property->getName(), property->getInstanceName(),
                property->getClassName(), property->getProcessName());
    if (!propertyBase->isRuntimeChange()) {
        throw PropertyNoRuntimeChangeSupported(propertyBase);
    }
    std::static_pointer_cast<Property<T>>(propertyBase)->setValue(value);
    if (m_mutablePropertyRepositories.empty()) {
        LOG_ERROR("no mutable property repositories available");
    } else {
        std::static_pointer_cast<Property<T>>(propertyBase)
                ->setDataStorage(m_mutablePropertyRepositories[0]->getDataStorage());
        for (const auto &item: m_mutablePropertyRepositories) {
            item->save(propertyBase);
        }
    }
    // publish after the repositories are updated, so that receivers resolving
    // a truncated value from shared memory read the new value
    publishChange(propertyBase);
}

#endif  // LOGGING_PROPERTYSERVICE_H
