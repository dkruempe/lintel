#ifndef CPP_BASE_LIBRARY_WEBSOCKETENTRY_H
#define CPP_BASE_LIBRARY_WEBSOCKETENTRY_H

#include <ostream>

#include "base_library/features/base/configuration/Entry.h"

class WebsocketEntry : public Entry {
 private:
  std::string address;
  uint16_t port;
  bool isServer;
  std::string name;

 public:
  WebsocketEntry(std::string_view component, std::string address, uint16_t port,
                 bool isServer, std::string name);

  [[nodiscard]] const std::string& getAddress() const;
  [[nodiscard]] uint16_t getPort() const;
  [[nodiscard]] bool isServer1() const;
  [[nodiscard]] const std::string& getName() const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const WebsocketEntry& entry);
};

#endif  // CPP_BASE_LIBRARY_WEBSOCKETENTRY_H
