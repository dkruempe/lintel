#ifndef CPP_BASE_LIBRARY_PERSISTENCE_IDENTIFIER_H
#define CPP_BASE_LIBRARY_PERSISTENCE_IDENTIFIER_H

#include <string>

namespace db {
    /** Quotes an SQL identifier (e.g. table or savepoint name) to prevent injection. */
    inline std::string quoteIdentifier(const std::string &identifier) {
        std::string quoted;
        quoted.reserve(identifier.size() + 2);
        quoted += '"';
        for (const char c: identifier) {
            if (c == '"') {
                quoted += "\"\"";
            } else {
                quoted += c;
            }
        }
        quoted += '"';
        return quoted;
    }
}  // namespace db

#endif  // CPP_BASE_LIBRARY_PERSISTENCE_IDENTIFIER_H
