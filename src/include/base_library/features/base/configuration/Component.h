#ifndef CPP_BASE_LIBRARY_COMPONENT_H
#define CPP_BASE_LIBRARY_COMPONENT_H

#include <memory>
#include <string>
#include <vector>

#include "Entry.h"

/** Base class for configuration parsing components */
class Component {
private:
    /** Root element name in the configuration XML for this component */
    std::string m_configRoot;

protected:
    /** Convert a human-readable size string to bytes
     * @param size Size string (e.g. "10MB", "1GB")
     * @return Size in bytes */
    static std::size_t convertToBytes(const std::string &size);

public:
    /** Construct a component with the given XML config root name
     * @param configRoot The root element name */
    explicit Component(std::string configRoot);

    /** Virtual destructor */
    virtual ~Component() = default;

    /** Get the configuration root element name
     * @return The config root string */
    [[nodiscard]] const std::string &getConfigRoot() const;

    /** Parse configuration content and produce entries
     * @param content The raw configuration content
     * @param fileName The source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed configuration entries */
    virtual std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                                       const std::string &fileName,
                                                       int32_t lineOffset) = 0;
};

#endif  // CPP_BASE_LIBRARY_COMPONENT_H
