#ifndef CPP_BASE_LIBRARY_INPUTSERVICE_H
#define CPP_BASE_LIBRARY_INPUTSERVICE_H

#include "base_library/features/cli/services/IInputService.h"

#include <termios.h>
#include <unistd.h>

class InputService : public IInputService
{
private:
  termios newt{};

public:
  InputService();

  ~InputService() override;

  KeyEvent onRead() override;
};

#endif// CPP_BASE_LIBRARY_INPUTSERVICE_H
