#ifndef CPP_BASE_LIBRARY_PACKET_H
#define CPP_BASE_LIBRARY_PACKET_H

#include <string>
#include <vector>

class Packet {
 private:
  std::string textBuffer;
  std::vector<uint8_t> binaryBuffer;
  bool text = false;

 public:
  explicit Packet(std::string textBuffer);
  explicit Packet(std::vector<uint8_t> binaryBuffer);

  [[nodiscard]] const std::string& getTextBuffer() const;
  [[nodiscard]] const std::vector<uint8_t>& getBinaryBuffer() const;
  [[nodiscard]] bool isText() const;
};

#endif  // CPP_BASE_LIBRARY_PACKET_H
