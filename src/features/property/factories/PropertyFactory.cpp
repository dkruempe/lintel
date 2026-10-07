#include "lintel/features/property/factories/PropertyFactory.h"

#include <chrono>
#include <cstdint>
#include <string>

#include "lintel/core/services/StringifyService.h"
#include "lintel/features/property/models/Property.h"

namespace {

template<typename T>
std::shared_ptr<PropertyBase> create(const std::string &name,
                                     const std::string &instanceName,
                                     const std::string &className,
                                     const std::string &processName,
                                     const std::string &value,
                                     const std::string &description,
                                     bool runtimeChange) {
    return std::make_shared<Property<T>>(
        name, instanceName, className, processName,
        StringifyService<T>::deserializeFromString(value), description,
        runtimeChange);
}

}  // namespace

std::shared_ptr<PropertyBase> PropertyFactory::Create(
        const std::string &name, const std::string &instanceName,
        const std::string &className, const std::string &processName,
        const std::string &type, const std::string &value,
        const std::string &description, bool runtimeChange) {
    if (type == "int8_t") return create<int8_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "int16_t") return create<int16_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "int32_t") return create<int32_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "int64_t") return create<int64_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "uint8_t") return create<uint8_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "uint16_t") return create<uint16_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "uint32_t") return create<uint32_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "uint64_t") return create<uint64_t>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "float") return create<float>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "double") return create<double>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "std::string") return create<std::string>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "bool") return create<bool>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "std::chrono::milliseconds") return create<std::chrono::milliseconds>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "std::chrono::seconds") return create<std::chrono::seconds>(name, instanceName, className, processName, value, description, runtimeChange);
    if (type == "std::chrono::minutes") return create<std::chrono::minutes>(name, instanceName, className, processName, value, description, runtimeChange);
    return nullptr;
}
