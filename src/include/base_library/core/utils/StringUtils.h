#ifndef CPP_BASE_LIBRARY_STRINGUTILS_H
#define CPP_BASE_LIBRARY_STRINGUTILS_H

#include <string>
#include <string_view>
#include <vector>

/**
 * Class for collecting of extra string utils
 */
class StringUtils {
public:
    StringUtils() = delete;

    StringUtils(StringUtils &) = delete;

    StringUtils(StringUtils &&) = delete;

    /**
     * function splits string with defined delimter into tokens
     * @param s string to be split
     * @param delimiter for differentiate between tokens
     * @return tokens in vector
     */
    static std::vector<std::string> split(const std::string &s, char delimiter);

    /** Check if a string starts with the given prefix.
     * @param s     the string to check
     * @param start the prefix to look for
     * @return true if s starts with start */
    static constexpr bool startsWith(std::string_view s, std::string_view start) {
        return s.size() >= start.size() && s.compare(0, start.size(), start) == 0;
    }

    /** Check if a string ends with the given suffix.
     * @param s   the string to check
     * @param end the suffix to look for
     * @return true if s ends with end */
    static constexpr bool endsWith(std::string_view s, std::string_view end) {
        return s.size() >= end.size() &&
               s.compare(s.size() - end.size(), end.size(), end) == 0;
    }

    /** Replace all occurrences of a substring with another string.
     * @param s       the input string
     * @param replace the substring to replace
     * @param with    the replacement string
     * @return the resulting string */
    static std::string replaceAll(const std::string &s, std::string &&replace,
                                  std::string &&with);
};

#endif  // CPP_BASE_LIBRARY_STRINGUTILS_H
