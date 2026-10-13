#ifndef LINTEL_IINPUTSERVICE_H
#define LINTEL_IINPUTSERVICE_H

#include "lintel/features/cli/CliTypes.h"

namespace lintel {
/** Interface for reading raw keyboard input */
class IInputService
{
public:
  virtual ~IInputService() = default;
  /** @return the next KeyEvent from input */
  virtual KeyEvent onRead() = 0;
};

}// namespace lintel
#endif
