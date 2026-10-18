#ifndef LINTEL_POOLNOTFOUNDEXCEPTION_H
#define LINTEL_POOLNOTFOUNDEXCEPTION_H

#include "lintel/core/exceptions/SQLException.h"

namespace lintel {
namespace db {
  /**
   * Thrown when @ref ConnectionPoolRegistry is asked for a pool that was never
   * registered.
   *
   * The registry never creates a pool implicitly: a name without a configured
   * `DatabaseConnectionEntry` (or an entry with `poolSize=0`) has no pool, and
   * asking for it is a programming or configuration error rather than a reason
   * to connect somewhere unexpected.
   */
  class PoolNotFoundException : public SQLException
  {
  public:
    /** @param message description of the missing pool */
    explicit PoolNotFoundException(std::string message) : SQLException(std::move(message)) {}
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_POOLNOTFOUNDEXCEPTION_H
