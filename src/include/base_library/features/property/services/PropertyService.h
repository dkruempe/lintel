#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include <functional>
#include <map>
#include <ostream>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/core/services/PersistableBean.h"
#include "base_library/features/base/models/ProcessName.h"
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

    // functions
    static std::string createIdentifier(const std::string &name,
                                        const std::string &instanceName,
                                        const std::string &className,
                                        const std::string &processName);

    static std::map<std::string, std::shared_ptr<PropertyBase>> init(
            const std::vector<std::shared_ptr<PropertyRepository>> &repoProperties,
            std::map<PropertyRepositoryType,
                    std::vector<std::shared_ptr<PropertyBase>>> &propertiesMap);

    static std::vector<std::shared_ptr<PropertyRepository>>
    filterEnabledRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepositories);

    static std::vector<std::shared_ptr<PropertyRepository>>
    filterMutableRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepository);

    static std::vector<std::shared_ptr<PropertyRepository>>
    filterShadowRepositories(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepository);

    void getOrCreate(
            const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec,
            std::map<PropertyRepositoryType,
                    std::vector<std::shared_ptr<PropertyBase>>> &map);

public:
    PropertyService(
            const std::vector<std::shared_ptr<PropertyRepository>>
            &propertyRepositories,
            std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices);

    virtual ~PropertyService() = default;

    void onAwake() override;

    void getOrCreate(
            const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec) {
        std::map<PropertyRepositoryType, std::vector<std::shared_ptr<PropertyBase>>>
                map;
        return getOrCreate(propertiesVec, map);
    }

    template<class T>
    std::shared_ptr<Property<T>> getOrCreate(const std::string &name,
                                             const std::string &instanceName,
                                             const std::string &className,
                                             const std::string &processName,
                                             const std::string &description,
                                             bool runtimeChange,
                                             const T &defaultValue = T());

    std::shared_ptr<PropertyBase> &get(const std::string &name,
                                       const std::string &instanceName,
                                       const std::string &className,
                                       const std::string &processName);

    std::vector<std::shared_ptr<PropertyBase>> allOf();

    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName);

    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName, const std::string &className);

    std::vector<std::shared_ptr<PropertyBase>> allOf(
            const std::string &processName, const std::string &className,
            const std::string &instanceName);

    template<class T>
    void changeValueOf(const std::shared_ptr<PropertyBase> &property,
                       const T &value);

    void changeStringValueOf(const std::shared_ptr<PropertyBase> &property,
                             const std::string &value);

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
            // no save of property but return property bc. its updated in cache
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
        return;
    }
    std::static_pointer_cast<Property<T>>(propertyBase)
            ->setDataStorage(m_mutablePropertyRepositories[0]->getDataStorage());
    for (const auto &item: m_mutablePropertyRepositories) {
        item->save(propertyBase);
    }
}

#endif  // LOGGING_PROPERTYSERVICE_H
