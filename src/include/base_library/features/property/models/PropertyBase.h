#ifndef LOGGING_PROPERTYBASE_H
#define LOGGING_PROPERTYBASE_H

#include <ostream>
#include <string>
#include <utility>

#include "base_library/features/property/models/DataStorage.h"

class PropertyBase {
 private:
  std::string m_name;
  std::string m_instanceName;
  std::string m_className;
  std::string m_processName;
  std::string m_identifier;
  const bool m_runtimeChange;
  const std::string m_description;
  DataStorage m_dataStorage;

 protected:
  PropertyBase(std::string name, std::string instanceName,
               std::string className, std::string processName,
               std::string description, bool runtimeChange)
      : m_name(std::move(name)),
        m_instanceName(std::move(instanceName)),
        m_className(std::move(className)),
        m_processName(std::move(processName)),
        m_identifier(m_name + "_" + m_instanceName + "_" +
                         m_className + "_" + m_processName),
        m_runtimeChange(runtimeChange),
        m_description(std::move(description)) {}

 public:
  PropertyBase() = delete;
  [[nodiscard]] virtual std::string toString() = 0;
  [[nodiscard]] virtual std::string getType() const = 0;
  [[nodiscard]] const std::string &getName() const { return m_name; }
  [[nodiscard]] const std::string &getInstanceName() const {
    return m_instanceName;
  }
  [[nodiscard]] const DataStorage &getDataStorage() const {
    return m_dataStorage;
  }
  void setDataStorage(const DataStorage &newDataStorage) {
    PropertyBase::m_dataStorage = newDataStorage;
  }
  [[nodiscard]] const std::string &getClassName() const { return m_className; }
  [[nodiscard]] const std::string &getProcessName() const {
    return m_processName;
  }
  [[nodiscard]] bool isRuntimeChange() const { return m_runtimeChange; }
  [[nodiscard]] const std::string &getDescription() const {
    return m_description;
  }
  [[nodiscard]] const std::string &getIdentifier() const { return m_identifier; }

  virtual void setValueString(const std::string &value) = 0;

  friend std::ostream &operator<<(std::ostream &os, PropertyBase &base) {
    os << "Property{"
       << "name:" << base.m_name << ", value:" << base.toString()
       << ", processName:" << base.m_processName
       << ", className:" << base.m_className
       << ", instanceName:" << base.m_instanceName
       << ", runtimeChange:" << base.m_runtimeChange
       << ", description:" << base.m_description
       << ", dataStorage:" << base.m_dataStorage << "}";
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
