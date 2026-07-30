#include "base_library/features/cli/services/InputService.h"

#include <csignal>
#include <cstdlib>
#include <mutex>

#include <cerrno>
#include <iostream>
#include <utility>

#include "base_library/core/services/LoggerService.h"

namespace {
    termios g_originalTermios{};

    void restoreTerminal() {
        if (g_originalTermios.c_cflag != 0) {
            tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
        }
    }

    bool g_sigusr1Registered = false;

    extern "C" void sigusr1Handler(int /*sig*/) {}
}

KeyEvent InputService::onRead()
{
    std::cout.flush();
    unsigned char ch;
    ssize_t n = ::read(STDIN_FILENO, &ch, 1);
    if (n < 0 && errno == EINTR) {
        return std::make_pair(KeyType::CtrlC, ' ');
    }
    if (n <= 0) {
        return std::make_pair(KeyType::CtrlC, ' ');
    }
    int chInt = static_cast<int>(ch);
    switch (chInt) {
    case 3:// CtrlC
        return std::make_pair(KeyType::CtrlC, ' ');
    case 4:// EOT
        return std::make_pair(KeyType::Eof, ' ');
    case 18:
        return std::make_pair(KeyType::CtrlR, ' ');
    case 127:
        return std::make_pair(KeyType::Backspace, ' ');
    case 10:
        return std::make_pair(KeyType::Ret, ' ');
        break;
    case 27:// symbol
        n = ::read(STDIN_FILENO, &ch, 1);
        if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
        chInt = static_cast<int>(ch);
        if (chInt == 91)// arrow keys
        {
            n = ::read(STDIN_FILENO, &ch, 1);
            if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
            chInt = static_cast<int>(ch);
            switch (chInt) {
            case 51:
                n = ::read(STDIN_FILENO, &ch, 1);
                if (n != 1) { return std::make_pair(KeyType::Ignored, ' '); }
                chInt = static_cast<int>(ch);
                if (chInt == 126) { return std::make_pair(KeyType::Canc, ' '); }
                return std::make_pair(KeyType::Ignored, ' ');
                break;
            case 65:
                return std::make_pair(KeyType::Up, ' ');
            case 66:
                return std::make_pair(KeyType::Down, ' ');
            case 68:
                return std::make_pair(KeyType::Left, ' ');
            case 67:
                return std::make_pair(KeyType::Right, ' ');
            case 70:
                return std::make_pair(KeyType::End, ' ');
            case 72:
                return std::make_pair(KeyType::Home, ' ');
            default:
                return std::make_pair(KeyType::Ignored, ' ');
            }
        }
        break;
    default:// ascii
    {
        const char c = static_cast<char>(chInt);
        return std::make_pair(KeyType::Ascii, c);
    }
    }
    return std::make_pair(KeyType::Ignored, ' ');
}

InputService::InputService() : newt()
{
    tcgetattr(STDIN_FILENO, &g_originalTermios);
    newt = g_originalTermios;
    newt.c_lflag &= ~tcflag_t(ICANON | ECHO | ISIG);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    static std::once_flag atexitFlag;
    std::call_once(atexitFlag, [] {
        std::atexit(restoreTerminal);
    });

    if (!g_sigusr1Registered) {
        g_sigusr1Registered = true;
        struct sigaction sa{};
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = sigusr1Handler;
        sigaction(SIGUSR1, &sa, nullptr);
    }
}

InputService::~InputService()
{
    restoreTerminal();
}
