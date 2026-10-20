#ifndef LINTEL_CORRELATIONID_H
#define LINTEL_CORRELATIONID_H

#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace lintel {

class CorrelationId
{
public:
  static std::string generate()
  {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    std::stringstream ss;
    ss << std::hex << dist(gen) << dist(gen);
    return ss.str();
  }
};

}// namespace lintel
#endif
