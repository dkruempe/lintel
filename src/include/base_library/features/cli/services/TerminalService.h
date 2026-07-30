#ifndef CPP_BASE_LIBRARY_TERMINALSERVICE_H
#define CPP_BASE_LIBRARY_TERMINALSERVICE_H

#include "base_library/features/cli/services/ITerminalService.h"

#include <string>

class TerminalService : public ITerminalService {
private:
    std::string m_currentLine;
    std::size_t m_position = 0;  // next writing position in currentLine
    bool m_hideChars = false;

public:
    void log(const std::string &text) override;

    SymbolEvent onKeyPressed(KeyEvent key, std::string menu) override;

    void resetCursor() override;

    const std::string &getLine() override;

    void enableHideChars() override;

    void disableHideChars() override;
};

#endif  // CPP_BASE_LIBRARY_TERMINALSERVICE_H
