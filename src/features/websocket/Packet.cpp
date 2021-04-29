#include "base_library/features/websocket/Packet.h"
Packet::Packet(std::string textBuffer)
    : m_textBuffer(std::move(textBuffer)), m_text(true) {}
Packet::Packet(std::vector<uint8_t> binaryBuffer)
    : m_binaryBuffer(std::move(binaryBuffer)) {}
const std::string& Packet::getTextBuffer() const { return m_textBuffer; }
const std::vector<uint8_t>& Packet::getBinaryBuffer() const {
  return m_binaryBuffer;
}
bool Packet::isText() const { return m_text; }
