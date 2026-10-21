#ifndef LINTEL_CORRELATIONID_H
#define LINTEL_CORRELATIONID_H

#include <cstddef>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace lintel {
/**
 * Factory for the correlation ids that match a response to its pending request.
 */
class CorrelationId
{
public:
  /** Buffer size of a correlation id including the terminating NUL (generate() yields at most 32 characters). */
  static constexpr std::size_t SIZE = 33;

  /**
   * @return random correlation id: two 64 bit random values as hex, at most 32 characters
   */
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
