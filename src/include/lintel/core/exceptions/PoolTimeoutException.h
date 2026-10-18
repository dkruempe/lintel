#ifndef LINTEL_POOLTIMEOUTEXCEPTION_H
#define LINTEL_POOLTIMEOUTEXCEPTION_H

#include "lintel/core/exceptions/SQLException.h"

namespace lintel {
namespace db {
  /**
   * Thrown when a @ref ConnectionPool cannot hand out a connection in time.
   *
   * The message names the pool and its configured maximum and lists every lease
   * that is still outstanding (source location, thread id, age), so a pool
   * exhaustion can be diagnosed from the exception alone. It is also thrown
   * immediately - without waiting - when the calling thread already holds a
   * lease of the same pool (re-entrancy) or when pooling is disabled
   * (`poolSize=0`).
   */
  class PoolTimeoutException : public SQLException
  {
  public:
    /** @param message description of the pool exhaustion */
    explicit PoolTimeoutException(std::string message) : SQLException(std::move(message)) {}
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_POOLTIMEOUTEXCEPTION_H
