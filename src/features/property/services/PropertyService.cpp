#include "lintel/features/property/services/PropertyService.h"

#include <algorithm>
#include <regex>
#include <sstream>

#include "lintel/features/property/exceptions/PropertyNotFoundException.h"
#include "lintel/features/property/models/PropertyBase.h"
#include "lintel/features/property/models/PropertyRepositoryType.h"

std::map<std::string, std::shared_ptr<PropertyBase>> PropertyService::init(
        const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories,
        std::map<PropertyRepositoryType, std::vector<std::shared_ptr<PropertyBase>>>
        &propertiesMap) {
    // sort repositories the highest repository first
    std::vector<std::shared_ptr<PropertyRepository>> repositories(
            propertyRepositories);
    std::sort(repositories.begin(), repositories.end(),
              [](const std::shared_ptr<PropertyRepository> &r1,
                 const std::shared_ptr<PropertyRepository> &r2) {
                  return r1->getType() < r2->getType();
              });

    // create local map store
    std::map<std::string, std::shared_ptr<PropertyBase>> propertiesMapLocal;

    // function to add properties to map
    std::function<void(std::vector<std::shared_ptr<PropertyBase>>)>
            addProperties = [&propertiesMapLocal](
            const std::vector<std::shared_ptr<PropertyBase>>
            &repoProperties) {
        for (const auto &property: repoProperties) {
            auto found = propertiesMapLocal.find(property->getIdentifier());
            if (found != propertiesMapLocal.end()) {
                found->second = property;
            } else {
                propertiesMapLocal.insert({property->getIdentifier(), property});
            }
        }
    };

    // add properties to map
    for (const std::shared_ptr<PropertyRepository> &repository: repositories) {
        std::vector<std::shared_ptr<PropertyBase>> tmp = repository->awake();
        propertiesMap.insert({repository->getType(), tmp});
        // highest property wins, bc. std::map insert only if not available
        addProperties(tmp);
    }
    return propertiesMapLocal;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterEnabledRepositories(
        const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories) {
    std::vector<std::shared_ptr<PropertyRepository>> tmp;
    for (const auto &iter: propertyRepositories) {
        if (!iter->isEnabled()) {
            continue;
        }
        tmp.push_back(iter);
    }
    return tmp;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterMutableRepositories(
        const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepository) {
    std::vector<std::shared_ptr<PropertyRepository>> repositories(
            propertyRepository);
    std::sort(repositories.begin(), repositories.end(),
              [](const std::shared_ptr<PropertyRepository> &r1,
                 const std::shared_ptr<PropertyRepository> &r2) {
                  return r1->getType() > r2->getType();
              });

    std::vector<std::shared_ptr<PropertyRepository>> mutableRepositories;
    for (const std::shared_ptr<PropertyRepository> &repository: repositories) {
        if (!repository->isEnabled()) {
            continue;
        }
        if (repository->isMutable()) {
            LOG_TRACE("mutableRepositories({})", repository->getType().toString());
            mutableRepositories.push_back(repository);
        }
    }

    return mutableRepositories;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterShadowRepositories(
        const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepository) {
    std::vector<std::shared_ptr<PropertyRepository>> repositories(
            propertyRepository);
    std::sort(repositories.begin(), repositories.end(),
              [](const std::shared_ptr<PropertyRepository> &r1,
                 const std::shared_ptr<PropertyRepository> &r2) {
                  return r1->getType() > r2->getType();
              });

    std::vector<std::shared_ptr<PropertyRepository>> shadowRepositories;
    for (const std::shared_ptr<PropertyRepository> &repository: repositories) {
        if (!repository->isEnabled()) {
            continue;
        }
        if (repository->isMutable() && repository->isShadow()) {
            LOG_TRACE("shadowRepositories({})", repository->getType().toString());
            shadowRepositories.push_back(repository);
        }
    }

    return shadowRepositories;
}

std::map<PropertyRepositoryType, std::shared_ptr<PropertyRepository>>
PropertyService::buildMap(
        const std::vector<std::shared_ptr<PropertyRepository>> &vector) {
    std::map<PropertyRepositoryType, std::shared_ptr<PropertyRepository>> map;
    for (const auto &item: vector) {
        map.insert({item->getType(), item});
    }
    return map;
}

PropertyService::PropertyService(
        const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories,
        std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices)
        : m_propertyRepositories(filterEnabledRepositories(propertyRepositories)),
          m_mutablePropertyRepositories(
                  filterMutableRepositories(propertyRepositories)),
          m_shadowPropertyRepositories(
                  filterShadowRepositories(propertyRepositories)),
          m_abstractServices(std::move(abstractServices)),
          m_typeRepositoryMap(buildMap(propertyRepositories)) {}

void PropertyService::onAwake() {
    // get properties from PropertyRepository
    std::map<PropertyRepositoryType, std::vector<std::shared_ptr<PropertyBase>>>
            map;
    m_properties = init(m_propertyRepositories, map);
    // get properties of AbstractServices
    std::vector<std::shared_ptr<PropertyBase>> properties;
    for (const auto &iter: m_abstractServices) {
        auto temp = iter->getProperties();
        properties.insert(properties.end(), temp.begin(), temp.end());
    }

    std::vector<std::shared_ptr<PropertyBase>> temp;
    for (const auto &[type, props]: map) {
        temp.insert(temp.end(), props.begin(), props.end());
    }
    bool reInit = false;
    for (const auto &iter: properties) {
        std::vector<std::shared_ptr<PropertyBase>> matches;
        std::copy_if(temp.begin(), temp.end(), std::back_inserter(matches),
                     [&](const std::shared_ptr<PropertyBase> &ptr) {
                         return ptr->getIdentifier() == iter->getIdentifier();
                     });
        if (matches.empty()) {
            // nothing to do
            continue;
        }
        matches.erase(
                std::remove_if(matches.begin(), matches.end(),
                               [&](const std::shared_ptr<PropertyBase> &property) {
                                   return property->toString() != iter->toString();
                               }),
                matches.end());
        if (matches.empty()) {
            // nothing to do
            continue;
        }
        // remove now all elements from repository if possible bc. no need of those
        for (const auto &property: matches) {
            auto found =
                    m_typeRepositoryMap.find(property->getDataStorage().getType());
            if (found == m_typeRepositoryMap.end()) {
                continue;
            }
            if (!found->second->isEnabled() || !found->second->isMutable()) {
                LOG_WARN(
                        "non mutable or disabled repository found with same value as the "
                        "default value for {} in {}",
                        iter->getIdentifier(), found->first.toString());
                continue;
            }
            if (found->second->isShadow()) {
                continue;
            }
            LOG_INFO("removed {} from {}", found->first.toString(),
                     property->getIdentifier());
            reInit = true;
            found->second->deleteOf({property});
        }
    }
    if (reInit) {
        LOG_INFO("reInit properties bc. of cleanup");
        m_properties = init(m_propertyRepositories, map);
    }
    // enable properties
    getOrCreate(properties, map);
}

std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf() {
    if (m_shadowPropertyRepositories.empty()) {
        std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
        std::transform(m_properties.begin(), m_properties.end(),
                       std::back_inserter(propertiesVector),
                       [](const auto &iter) { return iter.second; });
        return propertiesVector;
    }
    return m_shadowPropertyRepositories[0]->allOf();
}

std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
        const std::string &processName) {
    if (m_shadowPropertyRepositories.empty()) {
        std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
        std::transform(m_properties.begin(), m_properties.end(),
                       std::back_inserter(propertiesVector),
                       [](const auto &iter) { return iter.second; });
        return propertiesVector;
    }
    return m_shadowPropertyRepositories[0]->allOf(processName);
}

std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
        const std::string &processName, const std::string &className) {
    if (m_shadowPropertyRepositories.empty()) {
        std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
        std::regex processRegex(processName);
        std::regex classRegex(className);
        for (const auto &property: m_properties) {
            if (!std::regex_match(property.second->getProcessName(), processRegex)) {
                continue;
            }
            if (!std::regex_match(property.second->getClassName(), classRegex)) {
                continue;
            }
            propertiesVector.push_back(property.second);
        }
        return propertiesVector;
    }
    return m_shadowPropertyRepositories[0]->allOf(processName, className);
}

std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
        const std::string &processName, const std::string &className,
        const std::string &instanceName) {
    LOG_TRACE("shadowPropertyRepositories({})",
              m_shadowPropertyRepositories.size());
    if (m_shadowPropertyRepositories.empty()) {
        std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
        std::regex processRegex(processName);
        std::regex classRegex(className);
        std::regex instanceRegex(instanceName);
        LOG_TRACE("({},{},{}) properties({})", processName, className, instanceName,
                  m_properties.size());
        for (const auto &property: m_properties) {
            if (!std::regex_match(property.second->getProcessName(), processRegex)) {
                continue;
            }
            if (!std::regex_match(property.second->getClassName(), classRegex)) {
                continue;
            }
            if (!std::regex_match(property.second->getInstanceName(),
                                  instanceRegex)) {
                continue;
            }
            propertiesVector.push_back(property.second);
        }
        return propertiesVector;
    }
    return m_shadowPropertyRepositories[0]->allOf(processName, className,
                                                  instanceName);
}

std::string PropertyService::createIdentifier(const std::string &name,
                                              const std::string &instanceName,
                                              const std::string &className,
                                              const std::string &processName) {
    return name + "_" + instanceName + "_" + className + "_" + processName;
}

std::shared_ptr<PropertyBase> &PropertyService::get(
        const std::string &name, const std::string &instanceName,
        const std::string &className, const std::string &processName) {
    const std::string &identifier =
            createIdentifier(name, instanceName, className, processName);
    auto found = m_properties.find(identifier);
    if (found == m_properties.end()) {
        throw PropertyNotFoundException(name, instanceName, className, processName);
    }
    return found->second;
}

void PropertyService::getOrCreate(
        const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec,
        std::map<PropertyRepositoryType, std::vector<std::shared_ptr<PropertyBase>>>
        &map) {
    // enable properties
    for (const std::shared_ptr<PropertyBase> &property: propertiesVec) {
        try {
            auto &newProperty =
                    get(property->getName(), property->getInstanceName(),
                        property->getClassName(), property->getProcessName());
            // only overwrite value and Datastorage if value is different then default
            if (newProperty->toString() != property->toString()) {
                property->setValueString(newProperty->toString());
                property->setDataStorage(newProperty->getDataStorage());
            }
            newProperty = property;
        } catch (PropertyNotFoundException &exception) {
            m_properties.insert({property->getIdentifier(), property});
        }
    }
    // add missing properties to shadow repositories and cleanup not needed
    // properties from mutable repositories but not shadow
    for (auto &iter: m_shadowPropertyRepositories) {
        // create temp map for easier handling
        auto found = map.find(iter->getType());
        auto vec = found->second;
        std::map<std::string, std::shared_ptr<PropertyBase>> tmpMap;
        for (const auto &iterVec: vec) {
            tmpMap.insert({iterVec->getIdentifier(), iterVec});
        }
        if (!iter->isMutable() || !iter->isEnabled()) {
            continue;
        }
        std::vector<std::shared_ptr<PropertyBase>> properties = propertiesVec;
        properties.erase(
                std::remove_if(
                        properties.begin(), properties.end(),
                        [&](const std::shared_ptr<PropertyBase> &property) -> bool {
                            // don't compare value of property bc. each value change implies
                            // change of repository
                            return tmpMap.find(property->getIdentifier()) != tmpMap.end();
                        }),
                properties.end());
        iter->save(properties);
    }
}

void PropertyService::changeStringValueOf(
        const std::shared_ptr<PropertyBase> &property, const std::string &value) {
    std::shared_ptr<PropertyBase> propertyBase =
            get(property->getName(), property->getInstanceName(),
                property->getClassName(), property->getProcessName());
    if (!propertyBase->isRuntimeChange()) {
        throw PropertyNoRuntimeChangeSupported(propertyBase);
    }
    if (propertyBase->toString() == value) {
        return;
    }
    std::stringstream ss;
    ss << *property;
    LOG_INFO("{} change to {}", ss.str(), value);
    propertyBase->setValueString(value);
    if (m_mutablePropertyRepositories.empty()) {
        LOG_ERROR("{}: not mutable property repository",
                  propertyBase->getIdentifier());
    } else {
        // PropertyRepository with highest priority wins => DataStorage of the
        // highest priority is set
        propertyBase->setDataStorage(
                m_mutablePropertyRepositories[0]->getDataStorage());
        for (const auto &iter: m_mutablePropertyRepositories) {
            iter->save(propertyBase);
        }
    }
    // publish after the repositories are updated, so that receivers resolving
    // a truncated value from shared memory read the new value
    publishChange(propertyBase);
}

void PropertyService::publishChange(
        const std::shared_ptr<PropertyBase> &property) {
    if (m_changeBus == nullptr) {
        return;
    }
    Event event{std::string(PROPERTY_CHANGES_TOPIC),
                std::string(PROPERTY_CHANGES_TYPE)};
    event.assign(PropertyChange::of(*property));
    if (!m_changeBus->publishToSubscribers(event)) {
        LOG_WARN("property change for {} could not be delivered",
                 property->getIdentifier());
    }
}

void PropertyService::setPropertyChangeBus(
        const std::shared_ptr<IEventBus> &bus, const std::string &subscriberName) {
    m_changeBus = bus;
    m_changeSubscriber = subscriberName;
    if (m_changeBus == nullptr) {
        return;
    }
    m_changeBus->registerTopic(std::string(PROPERTY_CHANGES_TOPIC));
    if (!m_changeBus->subscribe(std::string(PROPERTY_CHANGES_TOPIC),
                                m_changeSubscriber)) {
        LOG_ERROR("could not subscribe {} to {}",
                  m_changeSubscriber, PROPERTY_CHANGES_TOPIC);
    }
}

bool PropertyService::applyChange(const PropertyChange &change) {
    try {
        auto &property = get(change.name, change.instanceName, change.className,
                             change.processName);
        if (!property->isRuntimeChange()) {
            LOG_WARN("ignoring change of {}: runtime change not supported",
                     change.identifier());
            return false;
        }
        std::string value = change.value;
        if (change.m_valueTruncated) {
            // the event payload cannot carry the full value: deliberately take
            // the lock and read the authoritative value from shared memory
            auto resolved = resolveChangeValue(change);
            if (!resolved) {
                return false;
            }
            value = *resolved;
        }
        if (property->toString() == value) {
            return false;
        }
        property->setValueString(value);
        LOG_INFO("property {} changed to {}", change.identifier(), value);
        return true;
    } catch (PropertyNotFoundException &exception) {
        LOG_WARN("received change for unknown property {}", change.identifier());
        return false;
    }
}

std::optional<std::string>
PropertyService::resolveChangeValue(const PropertyChange &change) {
    for (const auto &repository: m_propertyRepositories) {
        if (repository->getType() != PropertyRepositoryType::SHM_REPOSITORY) {
            continue;
        }
        std::vector<std::shared_ptr<PropertyBase>> candidates;
        try {
            candidates =
                    repository->allOf(change.processName, change.className,
                                      change.instanceName, change.name);
        } catch (const std::exception &exception) {
            LOG_ERROR("cannot resolve value of {}: {}", change.identifier(),
                      exception.what());
            return std::nullopt;
        }
        for (const auto &candidate: candidates) {
            if (candidate->getName() == change.name &&
                candidate->getInstanceName() == change.instanceName &&
                candidate->getClassName() == change.className &&
                candidate->getProcessName() == change.processName) {
                return candidate->toString();
            }
        }
        LOG_ERROR(
                "cannot resolve value of {}: property not found in shared memory",
                change.identifier());
        return std::nullopt;
    }
    LOG_ERROR(
            "cannot apply truncated change of {}: no shared memory repository "
            "configured",
            change.identifier());
    return std::nullopt;
}

std::size_t PropertyService::applyChangeNotifications() {
    if (m_changeBus == nullptr) {
        return 0;
    }
    std::size_t applied = 0;
    while (auto event = m_changeBus->receiveOf(m_changeSubscriber)) {
        if (event->topic() != PROPERTY_CHANGES_TOPIC ||
            event->type() != PROPERTY_CHANGES_TYPE) {
            continue;
        }
        if (applyChange(event->as<PropertyChange>())) {
            ++applied;
        }
    }
    return applied;
}