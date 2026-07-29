#ifndef LOGGING_PROPERTYFACTORY_H
#define LOGGING_PROPERTYFACTORY_H

#include <memory>
#include <string>

#include "base_library/features/property/models/PropertyBase.h"

class PropertyFactory {
public:
    PropertyFactory() = delete;

    static std::shared_ptr<PropertyBase> Create(
            const std::string &name, const std::string &instanceName,
            const std::string &className, const std::string &processName,
            const std::string &type, const std::string &value,
            const std::string &description, bool runtime);
};

#endif  // LOGGING_PROPERTYFACTORY_H
