#ifndef CPP_BASE_LIBRARY_IINPUTSERVICE_H
#define CPP_BASE_LIBRARY_IINPUTSERVICE_H

#include "base_library/features/cli/CliTypes.h"

class IInputService
{
public:
  virtual ~IInputService() = default;
  virtual KeyEvent onRead() = 0;
};

#endif
