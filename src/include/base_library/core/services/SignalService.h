#ifndef CPP_BASE_LIBRARY_SIGNALSERVICE_H
#define CPP_BASE_LIBRARY_SIGNALSERVICE_H

#include <csignal>
#include <unistd.h>

class SignalService {
public:
    static void raiseSignal(int32_t signal) {
        ::kill(::getpid(), signal);
    }

    static void kill(pid_t pid, int32_t signal) {
        ::kill(pid, signal);
    }
};

#endif  // CPP_BASE_LIBRARY_SIGNALSERVICE_H
