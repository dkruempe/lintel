#include "lintel/features/base/services/SharedMemoryRepositoryRegistry.h"

namespace lintel {

SharedMemoryRepositoryRegistry &SharedMemoryRepositoryRegistry::instance()
{
  static SharedMemoryRepositoryRegistry instance;
  return instance;
}

}// namespace lintel
