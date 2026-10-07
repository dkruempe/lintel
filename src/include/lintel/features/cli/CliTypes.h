#ifndef LINTEL_CLITYPES_H
#define LINTEL_CLITYPES_H

#include <string>
#include <utility>

/** Type of key event from terminal input */
enum class KeyType { Ascii, Up, Down, Left, Right, Backspace, Canc, Home, End, Ret, Eof, CtrlC, CtrlR, Ignored };
/** Pair of key type and ASCII character */
using KeyEvent = std::pair<KeyType, char>;

/** High-level symbol interpreted from key events */
enum class Symbol { Nothing, Command, Up, Down, Tab, Eof, CtrlC, CtrlR };
/** Pair of symbol and associated string data */
using SymbolEvent = std::pair<Symbol, std::string>;

#endif
