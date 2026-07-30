#ifndef CPP_BASE_LIBRARY_PROPERTYREPOSITORYCOMPONENT_H
#define CPP_BASE_LIBRARY_PROPERTYREPOSITORYCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/features/base/configuration/Component.h"

/** Parses property repository type configuration (mutable, shadow, type) from XML */
class PropertyRepositoryComponent : public Component {
private:
    static struct Shapes {
        const std::string CONFIG_ROOT = "PropertyRepositories";
        const std::string PROPERTY_REPOSITORY_ROOT = "PropertyRepository";
        const std::string PROPERTY_REPOSITORY_TYPE = "type";
        const std::string PROPERTY_REPOSITORY_MUTABLE = "mutable";
        const std::string PROPERTY_REPOSITORY_SHADOW = "shadow";
    } shape;

public:
    PropertyRepositoryComponent();

    /**
     * Parse XML content into property repository configuration entries
     * @param content XML content
     * @param fileName source file name
     * @param lineOffset line offset for error reporting
     * @return list of parsed entries
     */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                               const std::string &fileName,
                                               const int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYREPOSITORYCOMPONENT_H
