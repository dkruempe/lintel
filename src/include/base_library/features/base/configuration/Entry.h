#ifndef CPP_BASE_LIBRARY_CONFIG_ENTRY_H
#define CPP_BASE_LIBRARY_CONFIG_ENTRY_H

#include <ostream>
#include <string>

/** Base class for all configuration entries */
class Entry {
private:
    /** The name of the component that produced this entry */
    const std::string_view m_component;

public:
    /** Construct an entry with the given component name
     * @param component The configuration parser component name */
    explicit Entry(std::string_view component);

    /** Get the component name that produced this entry
     * @return The component name */
    constexpr std::string_view getConfigurationParserComponent() const { return m_component; }

    /** Stream insertion operator
     * @param os The output stream
     * @param entry The entry to output
     * @return The output stream */
    friend std::ostream &operator<<(std::ostream &os, const Entry &entry);
};

#endif  // CPP_BASE_LIBRARY_CONFIG_ENTRY_H
