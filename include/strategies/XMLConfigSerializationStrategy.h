#ifndef LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H
#define LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H

#include "strategies/ConfigSerializationStrategy.h"

class XMLConfigSerializationStrategy : public ConfigSerializationStrategy {
public:
  std::string serialize(std::vector<std::shared_ptr<PropertyBase>> properties) override;

  std::vector<std::shared_ptr<PropertyBase>> deserialize(const std::string &content) override;
};

#endif // LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H
