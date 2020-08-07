#ifndef LOGGING_CONFIGSERIALIZATIONSTRATEGY_H
#define LOGGING_CONFIGSERIALIZATIONSTRATEGY_H

#include "base_library/models/PropertyBase.h"
#include <memory>
#include <vector>

/**
 * Strategy for serialize/deserialize a list of properties or a property
 */
class ConfigSerializationStrategy {
public:
  /**
   * serialize Properties to string
   * @param properties
   * @return serialized Properties
   */
  virtual std::string
  serialize(std::vector<std::shared_ptr<PropertyBase>> properties) = 0;

  /**
   * deserialize a collection of properties
   * @param content serialized properties
   * @return deserialized properties in a vector
   */
  virtual std::vector<std::shared_ptr<PropertyBase>>
  deserialize(const std::string &content) = 0;
};

#endif // LOGGING_CONFIGSERIALIZATIONSTRATEGY_H
