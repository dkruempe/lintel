#ifndef LINTEL_COMMANDLINEUTILS_H
#define LINTEL_COMMANDLINEUTILS_H

#include <set>
#include <sstream>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)

#include <termios.h>
#include <unistd.h>

#endif

/** Utility class for terminal mode manipulation (echo, canonical mode) */
class CommandLineUtils {
private:
    bool m_isCanonicalMode = false;
    bool m_isEchoDisabled = false;

public:
    /** Disable input echo for password entry */
    void disableOfInputEcho();

    /** Re-enable input echo */
    void enableOfInputEcho();

    /** Clear the terminal screen */
    static void clear();

    /** Ring the terminal bell */
    static void beep();

    /** Disable canonical (line-buffered) input mode */
    void disableOfCanonicalMode();

    /** Re-enable canonical input mode */
    void enableOfCanonicalMode();

    /** @return true if canonical mode is active */
    bool isCanonicalMode() const;

    /** @return true if echo is enabled */
    bool isEchoMode() const;
};

#endif  // LINTEL_COMMANDLINEUTILS_H
