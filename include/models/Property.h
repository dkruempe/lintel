#ifndef PROPERTY_H
#define PROPERTY_H

#include "models/PropertyBase.h"
#include "services/StringifyService.h"
#include "utils/TypeName.h"
#include <string>

template <class T> class Property : public PropertyBase {
private:
  T value;

protected:
  virtual void setValue(const T &value) { this->value = value; }
  Property(const std::string &name, const std::string &instanceName,
           const std::string &processName, T value)
      : PropertyBase(name, instanceName, processName), value(std::move(value)) {
  }

public:
  Property(const std::string &name, const std::string &instanceName,
           const std::string &processName)
      : PropertyBase(name, instanceName, processName) {}
  [[nodiscard]] std::string toString() const override {
    return StringifyService<T>::serializeToString(value);
  }
  [[nodiscard]] std::string getType() const override {
    return std::string(type_name<T>());
  }
  T getValue() const { return value; }
};
#endif /* PROPERTY_H */
