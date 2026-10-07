#ifndef LOGGING_PROPERTYFACTORY_H
#define LOGGING_PROPERTYFACTORY_H

#include <memory>
#include <string>

#include "lintel/features/property/models/PropertyBase.h"

/** Factory for creating PropertyBase instances from string-typed values */
class PropertyFactory {
public:
    PropertyFactory() = delete;

    /**
     * Create a property from its string representation
     * @param name property name
     * @param instanceName instance name
     * @param className class name
     * @param processName process name
     * @param type the property type string
     * @param value the value as string
     * @param description property description
     * @param runtime whether runtime changes are supported
     * @return the created property
     */
    static std::shared_ptr<PropertyBase> Create(
            const std::string &name, const std::string &instanceName,
            const std::string &className, const std::string &processName,
            const std::string &type, const std::string &value,
            const std::string &description, bool runtime);
};

#endif  // LOGGING_PROPERTYFACTORY_H
