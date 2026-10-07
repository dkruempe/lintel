#ifndef LINTEL_PROPERTYENTRY_H
#define LINTEL_PROPERTYENTRY_H

#include <memory>
#include <ostream>

#include "lintel/features/base/configuration/Entry.h"
#include "lintel/features/property/factories/PropertyFactory.h"

/**
 * PropertyEntry is an possibility to define property values via a different
 * data storage. There is no possiblity to change this value via the property
 * service. This has to be done manually.
 */
class PropertyEntry : public Entry {
private:
    std::shared_ptr<PropertyBase> m_property;

public:
    /** @param component component name
     *  @param property the property to store */
    PropertyEntry(std::string_view component,
                  std::shared_ptr<PropertyBase> property);

    /** @return the stored property */
    std::shared_ptr<PropertyBase> &getProperty();

    friend std::ostream &operator<<(std::ostream &os, const PropertyEntry &entry);
};

#endif  // LINTEL_PROPERTYENTRY_H
