#ifndef LINTEL_SHAREDMEMORYREPOSITORY_H
#define LINTEL_SHAREDMEMORYREPOSITORY_H

#include <cstddef>
#include <string>

namespace lintel {

/** Basic typed SHM repository placeholder. */
template<typename T> class SharedMemoryRepository
{
public:
  SharedMemoryRepository() = default;
  virtual ~SharedMemoryRepository() = default;
};

}// namespace lintel
#endif// LINTEL_SHAREDMEMORYREPOSITORY_H
