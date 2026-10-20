#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

#include <string>

#include "lintel/features/base/services/ShmConstructors.h"

/**
 * Remove leftover named kernel objects of a crashed process.
 *
 * Every argument is removed as a named_upgradable_mutex (the historical
 * behaviour). The name is also treated as a shared-memory segment about to be
 * cleaned up: its remap semaphore and its lock-owner sidecar go with it, which
 * is the manual path for a lock the automatic recovery cannot decide on
 * (see shm::recoverOrphanedSegmentLock() and docs/architecture.dox).
 *
 * Only use it while no process uses those objects any more - a removed
 * semaphore orphans the handles of every running process.
 */
int main(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    const std::string name = argv[i];
    boost::interprocess::named_upgradable_mutex::remove(name.c_str());
    boost::interprocess::named_semaphore::remove(lintel::shm::remapSemaphoreNameOf(name).c_str());
    lintel::SegmentLockOwner::removeOf(name);
  }
  return 0;
}
