#ifndef CPP_BASE_LIBRARY_MOCKTERMINALSERVICE_H
#define CPP_BASE_LIBRARY_MOCKTERMINALSERVICE_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/cli/services/ITerminalService.h"

class MockTerminalService : public ITerminalService {
public:
    MAKE_MOCK1(log, void(const std::string &text), override);
    MAKE_MOCK2(onKeyPressed, SymbolEvent(KeyEvent key, std::string menu), override);
    MAKE_MOCK0(resetCursor, void(), override);
    MAKE_MOCK0(getLine, const std::string &(), override);
    MAKE_MOCK0(enableHideChars, void(), override);
    MAKE_MOCK0(disableHideChars, void(), override);
};

#endif
