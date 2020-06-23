#include "factories/PropertyFactory.h"

bool PropertyFactory::Register(const std::string &type,
                               TCreateMethod createMethod) {
  auto it = GetMap().find(type);
  if (it == GetMap().end()) {
    GetMap()[type] = std::move(createMethod);
    return true;
  }
  return false;
}

std::shared_ptr<PropertyBase> PropertyFactory::Create(const std::string &name,
                                                      const std::string &instanceName,
                                                      const std::string &processName,
                                                      const std::string &type,
                                                      const std::string &value) {
  auto it = GetMap().find(type);
  if (it != GetMap().end()) {
    return it->second(name, instanceName, processName, value);
  }

  return nullptr;
}

std::map<std::string, PropertyFactory::TCreateMethod> &
PropertyFactory::GetMap() {
  static std::map<std::string, TCreateMethod> s_methods;
  return s_methods;
}