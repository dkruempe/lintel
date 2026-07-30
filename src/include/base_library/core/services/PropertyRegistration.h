#ifndef PROPERTYREGISTRATION_H
#define PROPERTYREGISTRATION_H

#include <cstdint>
#include <filesystem>

#include "base_library/core/services/AbstractService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/property/models/Property.h"

template<class T>
class PropertyRegistration : public AbstractService<T> {
protected:
    template<class type>
    std::shared_ptr<Property<type>> registerProperty(
            std::string name, type defaultValue, std::string description,
            bool runtime, const std::string &fileName, int32_t position) {
        std::shared_ptr<Property<type>> property = std::make_shared<Property<type>>(
                name, this->getInstanceName(), std::string(this->getClassName()),
                this->getProcessName(), defaultValue, description, runtime);
        LOG_INFO("Property<{}> {} = {}", type_name<type>(), name,
                 property->toString());
        const std::filesystem::path &path(fileName);

        property->setDataStorage(DataStorage(
                PropertyRepositoryType::DEFAULT,
                path.filename().generic_string() + ":" + std::to_string(position)));
        this->m_properties.push_back(property);
        return property;
    }

public:
    using AbstractService<T>::AbstractService;
};

#endif  // PROPERTYREGISTRATION_H
