#ifndef LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H
#define LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H

#include "ConfigSerializationStrategy.h"

/** Strategy for serializing/deserializing properties to/from XML format */
class XMLConfigSerializationStrategy : public ConfigSerializationStrategy {
public:
    std::string serialize(
            std::vector<std::shared_ptr<PropertyBase>> properties) override;

    std::vector<std::shared_ptr<PropertyBase>> deserialize(
            const std::string &fileName, const std::string &content) override;

    std::vector<std::shared_ptr<PropertyBase>> deserialize(
            const std::filesystem::path &path) override;
};

#endif  // LOGGING_XMLCONFIGSERIALIZATIONSTRATEGY_H
