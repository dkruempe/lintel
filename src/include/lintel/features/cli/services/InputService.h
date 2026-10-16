#ifndef LINTEL_INPUTSERVICE_H
#define LINTEL_INPUTSERVICE_H

#include "lintel/features/cli/services/IInputService.h"

#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#endif

namespace lintel {
/** Reads raw terminal input by configuring the terminal to non-canonical mode */
class InputService : public IInputService
{
private:
#if !defined(_WIN32)
  termios newt{};
#endif

  /** Reads and decodes the rest of an ANSI escape sequence (after ESC). */
  KeyEvent readEscapeSequence();

public:
  InputService();

  ~InputService() override;

  KeyEvent onRead() override;
};

}// namespace lintel
#endif// LINTEL_INPUTSERVICE_H
