#ifndef LOGGING_ABSTRACTSERVICE_H
#define LOGGING_ABSTRACTSERVICE_H

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <base_library/core/models/ShutdownPriority.h>

#include "base_library/core/utils/TypeName.h"
class PropertyBase;

class PropertyService;

/** Interface that all services must implement for lifecycle management. */
class AbstractServiceInterface {
public:
    /** Called when the service should perform its initialization. */
    virtual void onInitialize() = 0;

    /** Called when the service should perform its shutdown. */
    virtual void onShutdown() = 0;

    /** Returns the shutdown priority of this service.
     * @return the shutdown priority */
    virtual ShutdownPriority shutdownPriorityOf() const = 0;

    /** Returns the class name of this service.
     * @return the class name as a string view */
    virtual std::string_view getClassName() const = 0;

    /** Returns the process name associated with this service.
     * @return the process name */
    virtual const std::string &getProcessName() const = 0;

    /** Returns the instance name of this service.
     * @return the instance name */
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

public:
    AbstractService() = delete;

    /** Construct an AbstractService with a process name and instance name.
     * @param processName  the process name
     * @param instanceName the instance name */
    AbstractService(std::string processName, std::string instanceName)
            : m_processName(std::move(processName)),
              m_instanceName(std::move(instanceName)) {}

    /** Construct an AbstractService with only a process name (instance defaults to "__DEFAULT").
     * @param processName the process name */
    explicit AbstractService(std::string processName)
            : m_processName(std::move(processName)), m_instanceName("__DEFAULT") {}

    /** Default initialization does nothing; override in subclasses. */
    void onInitialize() override {}

    /** Default shutdown does nothing; override in subclasses. */
    void onShutdown() override {}

    /** Returns the default shutdown priority.
     * @return ShutdownPriority::DEFAULT */
    ShutdownPriority shutdownPriorityOf() const override {
        return ShutdownPriority::DEFAULT;
    }

    /** Returns the process name.
     * @return the process name */
    [[nodiscard]] const std::string &getProcessName() const override {
        return m_processName;
    }

    /** Returns the instance name.
     * @return the instance name */
    [[nodiscard]] const std::string &getInstanceName() const override {
        return m_instanceName;
    }

    /** Returns the class name via type_name<T>().
     * @return the class name */
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
