#ifndef CPP_BASE_LIBRARY_IINPUTSERVICE_H
#define CPP_BASE_LIBRARY_IINPUTSERVICE_H

#include "base_library/features/cli/CliTypes.h"

/** Interface for reading raw keyboard input */
class IInputService
{
public:
  virtual ~IInputService() = default;
  /** @return the next KeyEvent from input */
  virtual KeyEvent onRead() = 0;
};

#endif
