#ifndef CPP_BASE_LIBRARY_UUID_H
#define CPP_BASE_LIBRARY_UUID_H

#include <string>

/** Utility class for generating universally unique identifiers (UUIDs). */
class UUID {
public:
    UUID() = delete;

    UUID(UUID &) = delete;

    UUID(UUID &&) = delete;

    /** Generate a new UUID string.
     * @return a UUID string (e.g. "550e8400-e29b-41d4-a716-446655440000") */
    static std::string generate();
};

#endif  // CPP_BASE_LIBRARY_UUID_H
