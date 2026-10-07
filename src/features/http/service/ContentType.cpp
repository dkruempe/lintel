#include "lintel/features/http/service/ContentType.h"

#include <algorithm>
#include <cctype>
#include <string_view>

ContentType::Value ContentType::build(std::string_view contentType) {
    if (contentType.empty()) {
        return UNDEFINED;
    }
    // strip parameters like "; charset=utf-8" and surrounding whitespace
    const std::size_t parameterPos = contentType.find(';');
    std::string_view mediaType =
            parameterPos == std::string::npos
                    ? contentType
                    : contentType.substr(0, parameterPos);
    while (!mediaType.empty() &&
           std::isspace(static_cast<unsigned char>(mediaType.front()))) {
        mediaType.remove_prefix(1);
    }
    while (!mediaType.empty() &&
           std::isspace(static_cast<unsigned char>(mediaType.back()))) {
        mediaType.remove_suffix(1);
    }
    for (const auto &mapping : kNameToValue) {
        if (mapping.name == mediaType) {
            return mapping.value;
        }
    }
    return UNDEFINED;
}

ContentType::ContentType(const std::string &contentType) : m_value(build(contentType)) {}

std::string ContentType::getName() const {
    for (const auto &mapping : kValueToName) {
        if (mapping.value == m_value) {
            return std::string(mapping.name);
        }
    }
    return "text/plain";
}
