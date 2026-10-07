#ifndef LINTEL_INPUTSERVICE_H
#define LINTEL_INPUTSERVICE_H

#include "lintel/features/cli/services/IInputService.h"

#include <termios.h>
#include <unistd.h>

/** Reads raw terminal input by configuring the terminal to non-canonical mode */
class InputService : public IInputService
{
private:
  termios newt{};

public:
  InputService();

  ~InputService() override;

  KeyEvent onRead() override;
};

#endif// LINTEL_INPUTSERVICE_H
