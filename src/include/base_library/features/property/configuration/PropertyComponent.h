#ifndef CPP_BASE_LIBRARY_PROPERTYCOMPONENT_H
#define CPP_BASE_LIBRARY_PROPERTYCOMPONENT_H

#include <tinyxml2.h>

#include "base_library/core/configuration/Component.h"

/** Parses property definitions from XML configuration */
class PropertyComponent : public Component {
private:
    static struct Shapes {
        const std::string CONFIG_ROOT = "Properties";
        const std::string ELEMENT_NAME = "name";
        const std::string PROCESS_ROOT = "process";
        const std::string CLASS_ROOT = "class";
        const std::string INSTANCE_ROOT = "instance";
        const std::string PROPERTY_ROOT = "Property";
        const std::string PROPERTY_TYPE = "type";
        const std::string PROPERTY_VALUE = "value";
    } shape;

public:
    PropertyComponent();

    /**
     * Parse XML content into property configuration entries
     * @param content XML content
     * @param fileName source file name
     * @param lineOffset line offset for error reporting
     * @return list of parsed entries
     */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                               const std::string &fileName,
                                               const int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYCOMPONENT_H
