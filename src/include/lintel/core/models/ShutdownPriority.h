#ifndef LINTEL_SHUTDOWNPRIORITY_H
#define LINTEL_SHUTDOWNPRIORITY_H

namespace lintel {
/**
 * Defines shutdown priority levels for process services.
 */
enum ShutdownPriority {
  DEFAULT = 0,
  PROCESS_SERVICE = 1,
};

}// namespace lintel
#endif// LINTEL_SHUTDOWNPRIORITY_H
