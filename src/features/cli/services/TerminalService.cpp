#include "lintel/features/cli/services/TerminalService.h"

#include <iostream>

void TerminalService::log(const std::string &text)
{
  if (!m_hideChars) { std::cout << std::string(m_position, '\b') << text << std::flush; }

  // if newLine is shorter than currentLine, we have
  // to clear the rest of the string
  if (text.size() < m_currentLine.size() && !m_hideChars) {
    std::cout << std::string(m_currentLine.size() - text.size(), ' ');
    // and go back
    std::cout << std::string(m_currentLine.size() - text.size(), '\b') << std::flush;
  }

  m_currentLine = text;
  m_position = m_currentLine.size();
}

SymbolEvent TerminalService::onKeyPressed(KeyEvent k, std::string menu)
{
  switch (k.first) {
  case KeyType::CtrlC:
    return { Symbol::CtrlC, {} };
  case KeyType::CtrlR:
    return { Symbol::CtrlR, {} };
  case KeyType::Eof:
    return std::make_pair(Symbol::Eof, std::string{});
    break;
  case KeyType::Backspace: {
    if (m_position == 0) { break; }
    --m_position;
    const auto pos = static_cast<std::string::difference_type>(m_position);
    // remove the char from buffer
    m_currentLine.erase(m_currentLine.begin() + pos);
    // go back to the previous char
    std::cout << '\b';
    // output the rest of the line
    std::cout << std::string(m_currentLine.begin() + pos, m_currentLine.end());
    // remove last char
    std::cout << ' ';
    // go back to the original position
    std::cout << std::string(m_currentLine.size() - m_position + 1, '\b') << std::flush;
    break;
  }
  case KeyType::Up:
    return std::make_pair(Symbol::Up, std::string{});
    break;
  case KeyType::Down:
    return std::make_pair(Symbol::Down, std::string{});
    break;
  case KeyType::Left:
    if (m_position <= 0) { break; }
    --m_position;
    std::cout << '\b' << std::flush;
    break;
  case KeyType::Right:
    if (m_position >= m_currentLine.size()) { break; }
    std::cout << m_currentLine[m_position] << std::flush;
    ++m_position;
    break;
  case KeyType::Ret: {
    std::cout << "\r\n";
    auto cmd = m_currentLine;
    m_currentLine.clear();
    m_position = 0;
    return std::make_pair(Symbol::Command, cmd);
  }
  case KeyType::Ascii: {
    const char c = static_cast<char>(k.second);
    if (c == '\t') { return std::make_pair(Symbol::Tab, m_currentLine); }
    const auto pos = static_cast<std::string::difference_type>(m_position);
    // output the new char:
    if (m_hideChars) {
      std::cout << '*';
    } else {
      std::cout << c;
    }
    // and the rest of the string:
    std::cout << std::string(m_currentLine.begin() + pos, m_currentLine.end());

    // go back to the original position
    std::cout << std::string(m_currentLine.size() - m_position, '\b') << std::flush;
    // update the buffer and cursor position:
    m_currentLine.insert(m_currentLine.begin() + pos, c);
    ++m_position;
    break;
  }
  case KeyType::Canc: {
    if (m_position == m_currentLine.size()) { break; }

    const auto pos = static_cast<std::string::difference_type>(m_position);

    // output the rest of the line
    std::cout << std::string(m_currentLine.begin() + pos + 1, m_currentLine.end());
    // remove last char
    std::cout << ' ';
    // go back to the original position
    std::cout << std::string(m_currentLine.size() - m_position, '\b') << std::flush;
    // remove the char from buffer
    m_currentLine.erase(m_currentLine.begin() + pos);
    break;
  }
  case KeyType::End: {
    const auto pos = static_cast<std::string::difference_type>(m_position);

    std::cout << std::string(m_currentLine.begin() + pos, m_currentLine.end()) << std::flush;
    m_position = m_currentLine.size();
    break;
  }
  case KeyType::Home: {
    std::cout << std::string(m_position, '\b') << std::flush;
    m_position = 0;
    break;
  }
  case KeyType::Ignored:
    break;
  }

  return std::make_pair(Symbol::Nothing, std::string());
}

void TerminalService::resetCursor()
{
  // move cursor size of line elements left
  for (std::size_t i = 0; i < m_currentLine.size(); ++i) { std::cout << '\b'; }
  // now override sequence with spaces
  for (std::size_t i = 0; i < m_currentLine.size(); ++i) { std::cout << ' '; }
  // now again backwards to new position
  for (std::size_t i = 0; i < m_currentLine.size(); ++i) { std::cout << '\b'; }
  // reset current line
  m_currentLine = "";
  m_position = 0;
}

const std::string &TerminalService::getLine() { return m_currentLine; }

void TerminalService::enableHideChars() { m_hideChars = true; }

void TerminalService::disableHideChars() { m_hideChars = false; }