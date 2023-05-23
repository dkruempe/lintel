#ifndef CPP_BASE_LIBRARY_UUID_H
#define CPP_BASE_LIBRARY_UUID_H

#include <string>

class UUID {
public:
    UUID() = delete;

    UUID(UUID &) = delete;

    UUID(UUID &&) = delete;

    static std::string generate();
};

#endif  // CPP_BASE_LIBRARY_UUID_H
