#include <cstddef>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <catch2/catch_all.hpp>

#include "lintel/features/base/controller/SharedMemorySegmentDto.h"

namespace lintel {
namespace {

  rapidjson::Document smsdValidDocument()
  {
    rapidjson::Document document;
    document.Parse(R"({
      "path":"/dev/shm/lintel_test.bin",
      "name":"lintel_test",
      "size":"1048576",
      "auto_extend":true,
      "auto_extend_size":"262144",
      "max_size":"4194304",
      "sanity":true,
      "current_size":"2097152",
      "free_size":"1048576",
      "named_objects":"3",
      "unique_objects":"2"
    })");
    return document;
  }

  std::string smsdSerialize(const JsonSerializable &serializable)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    serializable.serialize(&writer);
    return std::string(buffer.GetString());
  }

}// namespace

TEST_CASE("SharedMemorySegmentDto deserializes every field and round-trips")
{
  SharedMemorySegmentDto dto;
  REQUIRE(dto.deserialize(smsdValidDocument()));

  REQUIRE(dto.getPath() == std::filesystem::path("/dev/shm/lintel_test.bin"));
  REQUIRE(dto.getName() == "lintel_test");
  REQUIRE(dto.getSize() == 1048576u);
  REQUIRE(dto.isAutoExtend());
  REQUIRE(dto.getAutoExtendSize() == 262144u);
  REQUIRE(dto.getMaxSize() == 4194304u);
  REQUIRE(dto.isSanity());
  REQUIRE(dto.getCurrentSize() == 2097152u);
  REQUIRE(dto.getFreeSize() == 1048576u);
  REQUIRE(dto.getAmountNamedObjects() == 3u);
  REQUIRE(dto.getAmountUniqueObjects() == 2u);

  REQUIRE(&SharedMemorySegmentDto::getShape() != nullptr);

  const std::string json = smsdSerialize(dto);
  REQUIRE(json.find("\"name\":\"lintel_test\"") != std::string::npos);
  REQUIRE(json.find("\"auto_extend\":true") != std::string::npos);
  REQUIRE(json.find("\"named_objects\":\"3\"") != std::string::npos);
}

TEST_CASE("SharedMemorySegmentDto reports every missing field")
{
  const char *const keys[] = { "path",
    "name",
    "size",
    "auto_extend",
    "auto_extend_size",
    "max_size",
    "sanity",
    "current_size",
    "free_size",
    "named_objects",
    "unique_objects" };

  for (const char *key : keys) {
    SharedMemorySegmentDto dto;
    auto document = smsdValidDocument();
    document.RemoveMember(key);
    INFO("missing key: " << key);
    REQUIRE_FALSE(dto.deserialize(document));
  }
}

}// namespace lintel
