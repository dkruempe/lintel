#ifndef LINTEL_PERSISTENCE_IDENTIFIER_H
#define LINTEL_PERSISTENCE_IDENTIFIER_H

#include <string>
#include <string_view>

namespace db {
    /** Quotes an SQL identifier (e.g. table or savepoint name) to prevent injection. */
    inline std::string quoteIdentifier(std::string_view identifier) {
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

#endif  // LINTEL_PERSISTENCE_IDENTIFIER_H
