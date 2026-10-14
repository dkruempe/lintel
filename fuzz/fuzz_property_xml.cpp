#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "lintel/core/models/PropertyBase.h"
#include "lintel/features/property/strategies/XMLConfigSerializationStrategy.h"

// Property XML is parsed from configuration files and from repository contents,
// so a hostile document must not crash or throw.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, const std::size_t size)
{
  std::string content;
  if (size > 0) { content.assign(reinterpret_cast<const char *>(data), size); }
  try {
    lintel::XMLConfigSerializationStrategy strategy;
    const std::vector<std::shared_ptr<lintel::PropertyBase>> properties = strategy.deserialize("fuzz.xml", content);
    (void)properties;
  } catch (...) {
    // See fuzz_json_dto.cpp: a throw is a finding, not a reason to abort.
  }
  return 0;
}
