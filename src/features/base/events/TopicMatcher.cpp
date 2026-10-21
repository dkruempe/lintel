#include "lintel/features/base/events/TopicMatcher.h"

namespace lintel {

bool TopicMatcher::matches(const std::string &pattern, const std::string &topic)
{
  if (pattern == topic) { return true; }
  // Simple wildcard: * matches any sequence, ? matches single char
  size_t p = 0, t = 0;
  size_t star = std::string::npos, match = 0;
  while (t < topic.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == topic[t])) {
      ++p;
      ++t;
      continue;
    }
    if (p < pattern.size() && pattern[p] == '*') {
      star = p++;
      match = t;
      continue;
    }
    if (star != std::string::npos) {
      p = star + 1;
      t = ++match;
      continue;
    }
    return false;
  }
  while (p < pattern.size() && pattern[p] == '*') { ++p; }
  return p == pattern.size();
}

}// namespace lintel
