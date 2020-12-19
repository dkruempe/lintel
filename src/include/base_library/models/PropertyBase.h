#ifndef LOGGING_PROPERTYBASE_H
#define LOGGING_PROPERTYBASE_H

#include <ostream>
#include <string>
#include <utility>

#include "base_library/models/DataStorage.h"

class PropertyBase {
 private:
  std::string name;
  std::string instanceName;
  std::string className;
  std::string processName;
  std::string identifier;
  const bool runtimeChange;
  const std::string description;
  DataStorage dataStorage;

 protected:
  PropertyBase(std::string name, std::string instanceName,
               std::string className, std::string processName,
               std::string description, bool runtimeChange)
      : name(std::move(name)),
        instanceName(std::move(instanceName)),
        className(std::move(className)),
        processName(std::move(processName)),
        identifier(this->name + "_" + this->instanceName + "_" +
                   this->className + "_" + this->processName),
        runtimeChange(runtimeChange),
        description(std::move(description)) {}

 public:
  PropertyBase() = delete;
  [[nodiscard]] virtual std::string toString() = 0;
  [[nodiscard]] virtual std::string getType() const = 0;
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::string &getInstanceName() const {
    return instanceName;
  }
  [[nodiscard]] const DataStorage &getDataStorage() const {
    return dataStorage;
  }
  void setDataStorage(const DataStorage &newDataStorage) {
    PropertyBase::dataStorage = newDataStorage;
  }
  [[nodiscard]] const std::string &getClassName() const { return className; }
  [[nodiscard]] const std::string &getProcessName() const {
    return processName;
  }
  [[nodiscard]] bool isRuntimeChange() const { return runtimeChange; }
  [[nodiscard]] const std::string &getDescription() const {
    return description;
  }
  [[nodiscard]] const std::string &getIdentifier() const { return identifier; }

  virtual void setValueString(const std::string &value) = 0;

  friend std::ostream &operator<<(std::ostream &os, PropertyBase &base) {
    os << "Property{"
       << "name:" << base.name << ", value:" << base.toString()
       << ", processName:" << base.processName
       << ", className:" << base.className
       << ", instanceName:" << base.instanceName
       << ", runtimeChange:" << base.runtimeChange
       << ", description:" << base.description
       << ", dataStorage:" << base.dataStorage
       << "}";
    return os;
  }

  operator std::string() {
    std::ostringstream out;
    out << *this;
    return out.str();
  }

  bool operator<(const PropertyBase &rhs) const {
    if (getProcessName() < rhs.getProcessName()) {
      return true;
    }
    if (rhs.getProcessName() < getProcessName()) {
      return false;
    }
    if (getClassName() < rhs.getClassName()) {
      return true;
    }
    if (rhs.getClassName() < getClassName()) {
      return false;
    }
    return getInstanceName() < rhs.getInstanceName();
  }
};

#endif  // LOGGING_PROPERTYBASE_H
