#ifndef CPP_BASE_LIBRARY_ITERMINALSERVICE_H
#define CPP_BASE_LIBRARY_ITERMINALSERVICE_H

#include "base_library/features/cli/CliTypes.h"

#include <string>

class ITerminalService
{
public:
  virtual ~ITerminalService() = default;
  virtual void log(const std::string &text) = 0;
  virtual SymbolEvent onKeyPressed(KeyEvent key, std::string menu) = 0;
  virtual void resetCursor() = 0;
  virtual const std::string &getLine() = 0;
  virtual void enableHideChars() = 0;
  virtual void disableHideChars() = 0;
};

#endif
