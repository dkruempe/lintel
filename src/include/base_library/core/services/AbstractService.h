#ifndef LOGGING_ABSTRACTSERVICE_H
#define LOGGING_ABSTRACTSERVICE_H

#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <base_library/core/models/ShutdownPriority.h>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/property/models/Property.h"

class PropertyService;

class AbstractServiceInterface {
public:
    virtual void onInitialize() = 0;

    virtual void onShutdown() = 0;

    virtual ShutdownPriority shutdownPriorityOf() const = 0;

    virtual std::string_view getClassName() const = 0;

    virtual const std::string &getProcessName() const = 0;

    virtual const std::string &getInstanceName() const = 0;

    virtual ~AbstractServiceInterface() = default;

private:
    virtual std::vector<std::shared_ptr<PropertyBase>> getProperties() = 0;

    friend class PropertyService;
};

/**
 * Every Service must be derived from this service to make sure that basic
 * information gets provided
 *
 * These information are for example needed for initializing the Properties
 */
template<class T>
class AbstractService : public AbstractServiceInterface {
private:
    const std::string m_processName;
    const std::string m_instanceName;

    std::vector<std::shared_ptr<PropertyBase>> getProperties() override {
        return m_properties;
    }

protected:
    std::vector<std::shared_ptr<PropertyBase>> m_properties;

    template<class type>
    std::shared_ptr<Property<type>> registerProperty(
            std::string name, type defaultValue, std::string description,
            bool runtime, const std::string &fileName, int32_t position) {
        std::shared_ptr<Property<type>> property = std::make_shared<Property<type>>(
                name, getInstanceName(), std::string(getClassName()), getProcessName(),
                defaultValue, description, runtime);
        LOG_INFO("Property<{}> {} = {}", type_name<type>(), name,
                 property->toString());
        const std::filesystem::path &path(fileName);

        property->setDataStorage(DataStorage(
                PropertyRepositoryType::DEFAULT,
                path.filename().generic_string() + ":" + std::to_string(position)));
        m_properties.push_back(property);
        return property;
    }

public:
    AbstractService() = delete;

    AbstractService(std::string processName, std::string instanceName)
            : m_processName(std::move(processName)),
              m_instanceName(std::move(instanceName)) {}

    explicit AbstractService(std::string processName)
            : m_processName(std::move(processName)), m_instanceName("__DEFAULT") {}

    void onInitialize() override {}

    void onShutdown() override {}

    ShutdownPriority shutdownPriorityOf() const override {
        return ShutdownPriority::DEFAULT;
    }

    [[nodiscard]] const std::string &getProcessName() const override {
        return m_processName;
    }

    [[nodiscard]] const std::string &getInstanceName() const override {
        return m_instanceName;
    }

    [[nodiscard]] std::string_view getClassName() const override {
        return type_name<T>();
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const AbstractService &service) {
        os << "processName: " << service.m_processName
           << " instanceName: " << service.m_instanceName
           << " className: " << service.getClassName();
        return os;
    }
};

#endif  // LOGGING_ABSTRACTSERVICE_H
