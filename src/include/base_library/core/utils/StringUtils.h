#ifndef CPP_BASE_LIBRARY_STRINGUTILS_H
#define CPP_BASE_LIBRARY_STRINGUTILS_H

#include <string>
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

  static bool startsWith(const std::string &s, const std::string &start);
};

#endif  // CPP_BASE_LIBRARY_STRINGUTILS_H
