#ifndef CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H
#define CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H

#include <memory>
#include <ostream>

#include "base_library/core/configuration/Entry.h"
#include "base_library/core/property/PropertyRepositoryType.h"

/** Configuration entry for a property repository (type, mutability, shadow mode) */
class PropertyRepositoryEntry : public Entry {
private:
    PropertyRepositoryType m_type;
    bool m_isMutable;
    bool m_isShadow;

public:
    /** @param component component name
     *  @param type repository type
     *  @param isMutable whether the repository supports writes
     *  @param isShadow whether it is a shadow (read-only fallback) repository */
    PropertyRepositoryEntry(std::string_view component,
                            PropertyRepositoryType type, bool isMutable,
                            bool isShadow);

    /** @return true if the repository is mutable */
    [[nodiscard]] bool isMutable();

    /** @return true if the repository is a shadow */
    [[nodiscard]] bool isShadow();

    /** @return the repository type */
    [[nodiscard]] PropertyRepositoryType getType();

    friend std::ostream &operator<<(std::ostream &os,
                                    const PropertyRepositoryEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_PROPERTYREPOSITORYENTRY_H
