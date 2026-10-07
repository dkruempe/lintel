#ifndef LINTEL_ITERMINALSERVICE_H
#define LINTEL_ITERMINALSERVICE_H

#include "lintel/features/cli/CliTypes.h"

#include <string>

/** Interface for terminal display and line editing */
class ITerminalService
{
public:
  virtual ~ITerminalService() = default;
  /** Write text to the terminal output */
  virtual void log(const std::string &text) = 0;
  /** Process a key event in the context of the current menu */
  virtual SymbolEvent onKeyPressed(KeyEvent key, std::string menu) = 0;
  /** Reset cursor position for prompt refresh */
  virtual void resetCursor() = 0;
  /** @return the current input line buffer */
  virtual const std::string &getLine() = 0;
  /** Enable hidden character mode (e.g. for password entry) */
  virtual void enableHideChars() = 0;
  /** Disable hidden character mode */
  virtual void disableHideChars() = 0;
};

#endif
