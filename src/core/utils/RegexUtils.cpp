#include "lintel/core/utils/RegexUtils.h"

#include <regex>

namespace lintel {
namespace {

  std::size_t countWildcardRuns(const std::string &pattern)
  {
    std::size_t count = 0;
    for (std::size_t i = 0; i + 1 < pattern.size(); i++) {
      if (pattern[i] == '.') {
        if (pattern[i + 1] == '*' || pattern[i + 1] == '+') { count++; }
      }
    }
    return count;
  }

}// namespace

bool RegexUtils::validatePattern(const std::string &pattern, std::string &errorMessage)
{
  errorMessage.clear();
  if (pattern.empty()) {
    errorMessage = "empty pattern";
    return false;
  }
  if (pattern.size() > kMaxPatternLength) {
    errorMessage = "pattern exceeds maximum length of " + std::to_string(kMaxPatternLength);
    return false;
  }
  if (hasNestedQuantifier(pattern)) {
    errorMessage = "pattern contains nested quantifiers";
    return false;
  }
  if (countWildcardRuns(pattern) > 1) {
    errorMessage = "pattern contains too many wildcard sequences";
    return false;
  }
  try {
    std::regex re(pattern, std::regex::ECMAScript);
  } catch (const std::regex_error &exception) {
    errorMessage = "invalid pattern: " + std::string(exception.what());
    return false;
  }
  return true;
}

bool RegexUtils::matches(const std::string &pattern, const std::string &text)
{
  std::string errorMessage;
  if (!validatePattern(pattern, errorMessage)) { return false; }
  try {
    return std::regex_match(text, std::regex(pattern, std::regex::ECMAScript));
  } catch (const std::regex_error &) {
    return false;
  }
}

bool RegexUtils::hasNestedQuantifier(const std::string &pattern)
{
  std::size_t i = 0;
  while (i < pattern.size()) {
    if (pattern[i] != '(') {
      ++i;
      continue;
    }
    std::size_t depth = 1;
    std::size_t j = i + 1;
    bool innerHasQuantifier = false;
    while (j < pattern.size()) {
      if (pattern[j] == '\\') {
        // Skip the escaped character.
        j += 2;
      } else if (pattern[j] == '(') {
        depth++;
        ++j;
      } else if (pattern[j] == ')') {
        depth--;
        if (depth == 0) { break; }
        ++j;
      } else {
        if (pattern[j] == '*' || pattern[j] == '+' || pattern[j] == '{') { innerHasQuantifier = true; }
        ++j;
      }
    }
    if (j >= pattern.size()) {
      // unbalanced group – let the regex compiler reject it
      return false;
    }
    if (innerHasQuantifier && j + 1 < pattern.size()) {
      const char next = pattern[j + 1];
      if (next == '*' || next == '+' || next == '{') { return true; }
    }
    i = j + 1;
  }
  return false;
}
}// namespace lintel
