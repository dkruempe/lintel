#ifndef LOGGING_CONFIGSERIALIZATIONSTRATEGY_H
#define LOGGING_CONFIGSERIALIZATIONSTRATEGY_H

#include <filesystem>
#include <memory>
#include <vector>

#include "base_library/features/property/models/PropertyBase.h"

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
  deserialize(const std::string &fileName, const std::string &content) = 0;

  virtual std::vector<std::shared_ptr<PropertyBase>>
  deserialize(const std::filesystem::path &path) = 0;
};

#endif // LOGGING_CONFIGSERIALIZATIONSTRATEGY_H
