#ifndef CPP_BASE_LIBRARY_COMMANDLINEUTILS_H
#define CPP_BASE_LIBRARY_COMMANDLINEUTILS_H

#include <set>
#include <sstream>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)

#include <termios.h>
#include <unistd.h>

#endif

class CommandLineUtils {
private:
    bool m_isCanonicalMode = false;
    bool m_isEchoDisabled = false;

public:
    void disableOfInputEcho();

    void enableOfInputEcho();

    static void clear();

    static void beep();

    void disableOfCanonicalMode();

    void enableOfCanonicalMode();

    bool isCanonicalMode() const;

    bool isEchoMode() const;
};

#endif  // CPP_BASE_LIBRARY_COMMANDLINEUTILS_H
