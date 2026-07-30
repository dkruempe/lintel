#ifndef CPP_BASE_LIBRARY_SIGNALSERVICE_H
#define CPP_BASE_LIBRARY_SIGNALSERVICE_H

#include <csignal>
#include <unistd.h>

/** Utility service for sending OS signals to processes. */
class SignalService {
public:
    /** Send a signal to the current process.
     * @param signal the signal number to send */
    static void raiseSignal(int32_t signal) {
        ::kill(::getpid(), signal);
    }

    /** Send a signal to a specific process.
     * @param pid    the target process ID
     * @param signal the signal number to send */
    static void kill(pid_t pid, int32_t signal) {
        ::kill(pid, signal);
    }
};

#endif  // CPP_BASE_LIBRARY_SIGNALSERVICE_H
