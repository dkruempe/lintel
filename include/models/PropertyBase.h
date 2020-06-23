#ifndef LOGGING_PROPERTYBASE_H
#define LOGGING_PROPERTYBASE_H

#include <string>
#include <utility>

class PropertyBase {
private:
  std::string name;
  std::string instanceName;
  std::string processName;
  std::string identifier;

protected:
  PropertyBase(std::string name, std::string instanceName,
               std::string processName)
      : name(std::move(name)), instanceName(std::move(instanceName)),
        processName(std::move(processName)),
        identifier(this->name + "_" + this->instanceName + "_" +
                   this->processName) {}

public:
  PropertyBase() = delete;
  [[nodiscard]] virtual std::string toString() const = 0;
  [[nodiscard]] virtual std::string getType() const = 0;
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::string &getInstanceName() const {
    return instanceName;
  }
  [[nodiscard]] const std::string &getProcessName() const {
    return processName;
  }
  [[nodiscard]] const std::string &getIdentifier() const { return identifier; }
};

#endif // LOGGING_PROPERTYBASE_H
