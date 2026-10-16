#ifndef LINTEL_SIGNALSERVICE_H
#define LINTEL_SIGNALSERVICE_H

#include <csignal>
#include <cstdint>

#include "lintel/core/platform/Platform.h"

#if defined(_WIN32)
#include <windows.h>// OpenProcess / TerminateProcess (NOMINMAX set by Platform.h)
#endif// _WIN32

namespace lintel {
/** Utility service for sending OS signals to processes. */
class SignalService
{
public:
  /** Send a signal to the current process.
   * @param signal the signal number to send */
  static void raiseSignal(int32_t signal)
  {
#if defined(_WIN32)
    std::raise(signal);
#else
    ::kill(::getpid(), signal);
#endif
  }

  /** Send a signal to a specific process.
   * @param pid    the target process ID
   * @param signal the signal number to send */
  static void kill(lintel::ProcessId pid, int32_t signal)
  {
#if defined(_WIN32)
    static_cast<void>(signal);
    const HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (process == nullptr) { return; }
    static_cast<void>(TerminateProcess(process, 1));
    static_cast<void>(CloseHandle(process));
#else
    ::kill(static_cast<::pid_t>(pid), signal);
#endif
  }
};

}// namespace lintel
#endif// LINTEL_SIGNALSERVICE_H
