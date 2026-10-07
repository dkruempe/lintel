#include "lintel/features/property/configuration/PropertyRepositoryEntry.h"

#include "lintel/features/property/models/PropertyRepositoryType.h"

PropertyRepositoryEntry::PropertyRepositoryEntry(std::string_view component,
                                                 PropertyRepositoryType type,
                                                 bool isMutable, bool isShadow)
        : Entry(component),
          m_type(std::move(type)),
          m_isMutable(isMutable),
          m_isShadow(isShadow) {}

bool PropertyRepositoryEntry::isShadow() { return m_isShadow; }

bool PropertyRepositoryEntry::isMutable() { return m_isMutable; }

PropertyRepositoryType PropertyRepositoryEntry::getType() { return m_type; }

std::ostream &operator<<(std::ostream &os,
                         const PropertyRepositoryEntry &entry) {
    os << static_cast<const Entry &>(entry);
    return os;
}