#ifndef LOGGING_MUTABLEPROPERTY_H
#define LOGGING_MUTABLEPROPERTY_H

#include "models/Property.h"

template <class T> class MutableProperty : public Property<T> {
public:
  MutableProperty(const std::string &name, const std::string &instanceName,
                  const std::string &processName, T value)
      : Property<T>(name, instanceName, processName, value) {}

  void setValue(const T &value){
    Property<T>::setValue(value);
  }
};

#endif // LOGGING_MUTABLEPROPERTY_H
