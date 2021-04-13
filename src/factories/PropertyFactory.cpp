#include "base_library/factories/PropertyFactory.h"

bool PropertyFactory::Register(const std::string &type,
                               TCreateMethod createMethod) {
  auto it = GetMap().find(type);
  if (it == GetMap().end()) {
    GetMap()[type] = std::move(createMethod);
    return true;
  }
  return false;
}

std::shared_ptr<PropertyBase> PropertyFactory::Create(
    const std::string &name, const std::string &instanceName,
    const std::string &className, const std::string &processName,
    const std::string &type, const std::string &value,
    const std::string &description, bool runtimeChange) {
  auto it = GetMap().find(type);
  if (it != GetMap().end()) {
    return it->second(name, instanceName, className, processName, value,
                      description, runtimeChange);
  }

  return nullptr;
}

std::map<std::string, PropertyFactory::TCreateMethod> &
PropertyFactory::GetMap() {
  static std::map<std::string, TCreateMethod> s_methods;
  return s_methods;
}