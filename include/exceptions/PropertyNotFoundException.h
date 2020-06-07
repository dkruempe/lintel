#include <exception>

#ifndef LOGGING_PROPERTYNOTFOUNDEXCEPTION_H
#define LOGGING_PROPERTYNOTFOUNDEXCEPTION_H
#include <fmt/format.h>
#include <string>

class PropertyNotFoundException : public std::exception {
private:
  std::string name;
  std::string instanceName;
  std::string processName;
  std::string message;

public:
  PropertyNotFoundException(const std::string &name,
                            const std::string &instanceName,
                            const std::string &processName)
      : name(name), instanceName(instanceName), processName(processName),
        message(fmt::format("Property<> with name: {} instanceName: {} "
                            "processName: {} not found",
                            name, instanceName, processName)) {}

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::string &getInstanceName() const {
    return instanceName;
  }
  [[nodiscard]] const std::string &getProcessName() const {
    return processName;
  }
};

#endif // LOGGING_PROPERTYNOTFOUNDEXCEPTION_H
