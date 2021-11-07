#ifndef CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H
#define CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H

#include <memory>
#include <ostream>

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

class PropertyRepositoryEntry : public Entry {
 private:
  PropertyRepositoryType m_type;
  bool m_isMutable;
  bool m_isShadow;

 public:
  PropertyRepositoryEntry(std::string_view component,
                          PropertyRepositoryType type, bool isMutable,
                          bool isShadow);

  [[nodiscard]] bool isMutable();

  [[nodiscard]] bool isShadow();

  [[nodiscard]] PropertyRepositoryType getType();

  friend std::ostream &operator<<(std::ostream &os,
                                  const PropertyRepositoryEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H
