#include "base_library/features/cli/services/TerminalService.h"

#include <iostream>

#include "base_library/core/services/LoggerService.h"
void TerminalService::log(const std::string &text) {
  std::cout << std::string(m_position, '\b') << text << std::flush;

  // if newLine is shorter then currentLine, we have
  // to clear the rest of the string
  if (text.size() < m_currentLine.size()) {
    std::cout << std::string(m_currentLine.size() - text.size(), ' ');
    // and go back
    std::cout << std::string(m_currentLine.size() - text.size(), '\b')
              << std::flush;
  }

  m_currentLine = text;
  m_position = m_currentLine.size();
}
SymbolEvent TerminalService::onKeyPressed(KeyEvent k) {
  switch (k.first) {
    case KeyType::CtrlC:
      return {Symbol::CtrlC, {}};
    case KeyType::Eof:
      return std::make_pair(Symbol::Eof, std::string{});
      break;
    case KeyType::Backspace: {
      if (m_position == 0) break;

      --m_position;

      const auto pos = static_cast<std::string::difference_type>(m_position);
      // remove the char from buffer
      m_currentLine.erase(m_currentLine.begin() + pos);
      // go back to the previous char
      std::cout << '\b';
      // output the rest of the line
      std::cout << std::string(m_currentLine.begin() + pos,
                               m_currentLine.end());
      // remove last char
      std::cout << ' ';
      // go back to the original position
      std::cout << std::string(m_currentLine.size() - m_position + 1, '\b')
                << std::flush;
      break;
    }
    case KeyType::Up:
      return std::make_pair(Symbol::Up, std::string{});
      break;
    case KeyType::Down:
      return std::make_pair(Symbol::Down, std::string{});
      break;
    case KeyType::Left:
      if (m_position > 0) {
        std::cout << '\b' << std::flush;
        --m_position;
      }
      break;
    case KeyType::Right:
      if (m_position < m_currentLine.size()) {
        std::cout << m_currentLine[m_position] << std::flush;
        ++m_position;
      }
      break;
    case KeyType::Ret: {
      std::cout << "\r\n";
      auto cmd = m_currentLine;
      m_currentLine.clear();
      m_position = 0;
      return std::make_pair(Symbol::Command, cmd);
    } break;
    case KeyType::Ascii: {
      const char c = static_cast<char>(k.second);
      if (c == '\t') {
        return std::make_pair(Symbol::Tab, m_currentLine);
      } else {
        const auto pos = static_cast<std::string::difference_type>(m_position);
        // output the new char:
        std::cout << c;
        // and the rest of the string:
        std::cout << std::string(m_currentLine.begin() + pos,
                                 m_currentLine.end());

        // go back to the original position
        std::cout << std::string(m_currentLine.size() - m_position, '\b')
                  << std::flush;

        // update the buffer and cursor position:
        m_currentLine.insert(m_currentLine.begin() + pos, c);
        ++m_position;
      }

      break;
    }
    case KeyType::Canc: {
      if (m_position == m_currentLine.size()) {
        break;
      }

      const auto pos = static_cast<std::string::difference_type>(m_position);

      // output the rest of the line
      std::cout << std::string(m_currentLine.begin() + pos + 1,
                               m_currentLine.end());
      // remove last char
      std::cout << ' ';
      // go back to the original position
      std::cout << std::string(m_currentLine.size() - m_position, '\b')
                << std::flush;
      // remove the char from buffer
      m_currentLine.erase(m_currentLine.begin() + pos);
      break;
    }
    case KeyType::End: {
      const auto pos = static_cast<std::string::difference_type>(m_position);

      std::cout << std::string(m_currentLine.begin() + pos, m_currentLine.end())
                << std::flush;
      m_position = m_currentLine.size();
      break;
    }
    case KeyType::Home: {
      std::cout << std::string(m_position, '\b') << std::flush;
      m_position = 0;
      break;
    }
    case KeyType::Ignored:
      // TODO
      break;
  }

  return std::make_pair(Symbol::Nothing, std::string());
}
void TerminalService::resetCursor() { m_position = 0; }
const std::string &TerminalService::getLine() { return m_currentLine; }
