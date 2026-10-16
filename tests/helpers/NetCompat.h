#ifndef LINTEL_TESTS_NETCOMPAT_H
#define LINTEL_TESTS_NETCOMPAT_H

// Portable "ask the kernel for a free TCP port" helper shared by the socket-based tests. The
// alternative - guarding every such test with #if !defined(_WIN32) - would drop the HTTP server
// tests on Windows, which are exactly the ones that prove the API is usable there.

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif// _WIN32

namespace lintel {
namespace testcompat {

  /**
   * Bind a TCP socket to port 0 on 127.0.0.1 and read back the kernel-assigned port. The socket is
   * closed right away, so there is a narrow window in which another process could take the port;
   * callers must treat a failure to bind as a hard error.
   * @return a free port number, or -1 if the socket could not be created or bound
   */
  inline int freeTcpPort()
  {
#if defined(_WIN32)
    WSADATA wsaData{};
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { return -1; }
    const SOCKET sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
      ::WSACleanup();
      return -1;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    int port = -1;
    if (::bind(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0) {
      int length = sizeof(address);
      if (::getsockname(sock, reinterpret_cast<sockaddr *>(&address), &length) == 0) { port = ntohs(address.sin_port); }
    }
    ::closesocket(sock);
    ::WSACleanup();
    return port;
#else
    // Not written as ::htonl/::ntohs: macOS defines both as macros, and the
    // leading "::" turns the expansion into a syntax error there.
    const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { return -1; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    int port = -1;
    if (::bind(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0) {
      socklen_t length = sizeof(address);
      if (::getsockname(sock, reinterpret_cast<sockaddr *>(&address), &length) == 0) { port = ntohs(address.sin_port); }
    }
    ::close(sock);
    return port;
#endif// _WIN32
  }

}// namespace testcompat
}// namespace lintel

#endif// LINTEL_TESTS_NETCOMPAT_H
