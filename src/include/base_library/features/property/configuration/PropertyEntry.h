#ifndef CPP_BASE_LIBRARY_PROPERTYENTRY_H
#define CPP_BASE_LIBRARY_PROPERTYENTRY_H

#include <memory>
#include <ostream>

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/property/factories/PropertyFactory.h"

/**
 * PropertyEntry is an possibility to define property values via a different
 * data storage. There is no possiblity to change this value via the property
 * service. This has to be done manually.
 */
class PropertyEntry : public Entry {
private:
  std::shared_ptr<PropertyBase> property;

public:
  PropertyEntry(std::string_view component,
                std::shared_ptr<PropertyBase> property);

  std::shared_ptr<PropertyBase> &getProperty();

  friend std::ostream &operator<<(std::ostream &os, const PropertyEntry &entry);
};

#endif // CPP_BASE_LIBRARY_PROPERTYENTRY_H
