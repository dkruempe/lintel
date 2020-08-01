#ifndef LOGGING_PROPERTYFACTORY_H
#define LOGGING_PROPERTYFACTORY_H

#include "base_library/models/PropertyBase.h"
#include <functional>
#include <map>
#include <string>

class PropertyFactory {
public:
  typedef std::function<std::shared_ptr<PropertyBase>(
      const std::string &name, const std::string &instanceName,
      const std::string &className, const std::string &processName,
      const std::string &value)>
      TCreateMethod;

  PropertyFactory() = delete;
  ~PropertyFactory() = default;

  static bool Register(const std::string &type, TCreateMethod createMethod);

  static std::shared_ptr<PropertyBase>
  Create(const std::string &name, const std::string &instanceName,
         const std::string &className, const std::string &processName,
         const std::string &type, const std::string &value);

  static std::map<std::string, TCreateMethod> &GetMap();
};
#endif // LOGGING_PROPERTYFACTORY_H
