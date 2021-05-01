#include "base_library/features/websocket/configuration/WebsocketEntry.h"

#include <utility>
const std::string& WebsocketEntry::getAddress() const { return m_address; }
uint16_t WebsocketEntry::getPort() const { return m_port; }
WebsocketEntry::WebsocketEntry(std::string_view component, std::string address,
                               const uint16_t port, bool isServer,
                               std::string name)
    : Entry(component),
      m_address(std::move(address)),
      m_port(port),
      m_isServer(isServer),
      m_name(std::move(name)) {}
std::ostream& operator<<(std::ostream& os, const WebsocketEntry& entry) {
  os << static_cast<const Entry&>(entry) << " address: " << entry.m_address
     << " port: " << entry.m_port << " isServer: " << entry.m_isServer
     << " name: " << entry.m_name;
  return os;
}
bool WebsocketEntry::isServer1() const { return m_isServer; }
const std::string& WebsocketEntry::getName() const { return m_name; }
