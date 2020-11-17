#ifndef LOGGING_PROPERTYBASE_H
#define LOGGING_PROPERTYBASE_H

#include <ostream>
#include <string>
#include <utility>

class PropertyBase {
 private:
  std::string name;
  std::string instanceName;
  std::string className;
  std::string processName;
  std::string identifier;

 protected:
  PropertyBase(std::string name, std::string instanceName,
               std::string className, std::string processName)
      : name(std::move(name)),
        instanceName(std::move(instanceName)),
        className(std::move(className)),
        processName(std::move(processName)),
        identifier(this->name + "_" + this->instanceName + "_" +
                   this->className + "_" + this->processName) {}

 public:
  PropertyBase() = delete;
  [[nodiscard]] virtual std::string toString() = 0;
  [[nodiscard]] virtual std::string getType() const = 0;
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::string &getInstanceName() const {
    return instanceName;
  }
  [[nodiscard]] const std::string &getClassName() const { return className; }
  [[nodiscard]] const std::string &getProcessName() const {
    return processName;
  }
  [[nodiscard]] const std::string &getIdentifier() const { return identifier; }

  virtual void setValueString(const std::string &value) = 0;

  friend std::ostream &operator<<(std::ostream &os, PropertyBase &base) {
    os << "name: " << base.name << " instanceName: " << base.instanceName
       << " className: " << base.className
       << " processName: " << base.processName << " type: " << base.getType()
       << " value: " << base.toString();
    return os;
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
