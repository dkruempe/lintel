#ifndef CPP_BASE_LIBRARY_TERMINALSERVICE_H
#define CPP_BASE_LIBRARY_TERMINALSERVICE_H

#include <base_library/features/cli/services/InputService.h>

#include <string>
#include <utility>

enum class Symbol { Nothing, Command, Up, Down, Tab, Eof };
using SymbolEvent = std::pair<Symbol, std::string>;

class TerminalService {
 private:
  std::string m_currentLine;
  std::size_t m_position = 0; // next writing position in currentLine

 public:
  void log(const std::string &text);
  SymbolEvent onKeyPressed(KeyEvent);
  void resetCursor();
  const std::string &getLine();
};

#endif  // CPP_BASE_LIBRARY_TERMINALSERVICE_H
