#include "lintel/core/shm/SharedMemoryRepositoryRegistry.h"

namespace lintel {

SharedMemoryRepositoryRegistry &SharedMemoryRepositoryRegistry::instance()
{
  static SharedMemoryRepositoryRegistry instance;
  return instance;
}

}// namespace lintel
