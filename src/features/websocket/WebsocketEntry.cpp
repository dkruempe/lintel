#include "base_library/features/websocket/WebsocketEntry.h"

#include <utility>
const std::string& WebsocketEntry::getAddress() const { return address; }
uint16_t WebsocketEntry::getPort() const { return port; }
WebsocketEntry::WebsocketEntry(std::string_view component, std::string address,
                               const uint16_t port, bool isServer,
                               std::string name)
    : Entry(component),
      address(std::move(address)),
      port(port),
      isServer(isServer),
      name(std::move(name)) {}
std::ostream& operator<<(std::ostream& os, const WebsocketEntry& entry) {
  os << static_cast<const Entry&>(entry) << " address: " << entry.address
     << " port: " << entry.port << " isServer: " << entry.isServer
     << " name: " << entry.name;
  return os;
}
bool WebsocketEntry::isServer1() const { return isServer; }
const std::string& WebsocketEntry::getName() const { return name; }
