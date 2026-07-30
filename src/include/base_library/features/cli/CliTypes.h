#ifndef CPP_BASE_LIBRARY_CLITYPES_H
#define CPP_BASE_LIBRARY_CLITYPES_H

#include <string>
#include <utility>

enum class KeyType { Ascii, Up, Down, Left, Right, Backspace, Canc, Home, End, Ret, Eof, CtrlC, CtrlR, Ignored };
using KeyEvent = std::pair<KeyType, char>;

enum class Symbol { Nothing, Command, Up, Down, Tab, Eof, CtrlC, CtrlR };
using SymbolEvent = std::pair<Symbol, std::string>;

#endif
