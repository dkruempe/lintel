#include "base_library/features/websocket/Packet.h"
Packet::Packet(std::string textBuffer)
    : textBuffer(std::move(textBuffer)), text(true) {}
Packet::Packet(std::vector<uint8_t> binaryBuffer)
    : binaryBuffer(std::move(binaryBuffer)) {}
const std::string& Packet::getTextBuffer() const { return textBuffer; }
const std::vector<uint8_t>& Packet::getBinaryBuffer() const {
  return binaryBuffer;
}
bool Packet::isText() const { return text; }
