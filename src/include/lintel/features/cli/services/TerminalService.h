#ifndef LINTEL_TERMINALSERVICE_H
#define LINTEL_TERMINALSERVICE_H

#include "lintel/features/cli/services/ITerminalService.h"

#include <string>

/** Implementation of ITerminalService for line editing and display */
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

#endif  // LINTEL_TERMINALSERVICE_H
