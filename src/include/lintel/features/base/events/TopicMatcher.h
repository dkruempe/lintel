#ifndef LINTEL_TOPICMATCHER_H
#define LINTEL_TOPICMATCHER_H

#include <string>
#include <vector>

namespace lintel {

class TopicMatcher
{
public:
  static bool matches(const std::string &pattern, const std::string &topic);
};

}// namespace lintel
#endif// LINTEL_TOPICMATCHER_H
