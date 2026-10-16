#ifndef LINTEL_TESTS_PLATFORMCOMPAT_H
#define LINTEL_TESTS_PLATFORMCOMPAT_H

// Small portability shim for the test suite. The tests are written against POSIX; on MSVC this
// header supplies the handful of names they use (currently getpid()/pid_t, which every fixture uses
// to build unique file, segment and mutex names). Individual test cases that cannot work on Windows
// at all (sockets, pipes, fork()) guard their own bodies with `#if !defined(_WIN32)`.

#if defined(_WIN32)
#include <process.h>// _getpid

/** POSIX pid_t counterpart for MSVC. */
using pid_t = int;

/** Portable counterpart of POSIX getpid(); the tests only use it to build unique names. */
inline pid_t getpid() { return static_cast<pid_t>(::_getpid()); }
#else
#include <sys/types.h>// pid_t
#include <unistd.h>// getpid
#endif// _WIN32

#endif// LINTEL_TESTS_PLATFORMCOMPAT_H
