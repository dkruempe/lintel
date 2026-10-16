#include <cstdint>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <catch2/catch_all.hpp>

#include "lintel/features/base/controller/ProcessInfoDto.h"

namespace lintel {
namespace {

  rapidjson::Document pidDtoValidDocument()
  {
    rapidjson::Document document;
    document.Parse(R"({
      "id":"process-1",
      "path":"/usr/bin/sleep",
      "args":[{"arg":"10"},{"arg":"--verbose"}],
      "auto_restart":true,
      "restarts":3,
      "max_restarts":10,
      "process_id":4242,
      "runs":true,
      "exit_code":0,
      "exit_code_valid":true,
      "group_name":"workers",
      "group_id":"group-uuid",
      "resources_valid":true,
      "uptime_seconds":42,
      "cpu_percent":12.5,
      "memory_bytes":2048
    })");
    return document;
  }

  std::string pidDtoSerialize(const JsonSerializable &serializable)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    serializable.serialize(&writer);
    return std::string(buffer.GetString());
  }

}// namespace

TEST_CASE("ProcessInfoDto deserializes every field and round-trips")
{
  ProcessInfoDto dto;
  REQUIRE(dto.deserialize(pidDtoValidDocument()));

  REQUIRE(dto.getId() == "process-1");
  REQUIRE(dto.getPath() == std::filesystem::path("/usr/bin/sleep"));
  REQUIRE(dto.getArgs() == std::vector<std::string>{ "10", "--verbose" });
  REQUIRE(dto.isAutoRestart());
  REQUIRE(dto.getRestarts() == 3);
  REQUIRE(dto.getMaxAutoRestarts() == 10);
  REQUIRE(dto.getProcessId() == 4242);
  REQUIRE(dto.isRunning());
  REQUIRE(dto.getExitCode() == 0);
  REQUIRE(dto.isExitCodeValid());
  REQUIRE(dto.getGroupName() == "workers");
  REQUIRE(dto.getGroupId() == "group-uuid");
  REQUIRE(dto.isResourceDataValid());
  REQUIRE(dto.getUptimeSeconds().value() == 42);
  REQUIRE(dto.getCpuPercent().value() == Catch::Approx(12.5));
  REQUIRE(dto.getMemoryBytes().value() == 2048u);

  const std::string json = pidDtoSerialize(dto);
  REQUIRE(json.find("\"cpu_percent\":12.5") != std::string::npos);
  REQUIRE(json.find("\"memory_bytes\":2048") != std::string::npos);
  REQUIRE(json.find("\"uptime_seconds\":42") != std::string::npos);
  REQUIRE(json.find("\"group_name\":\"workers\"") != std::string::npos);
}

TEST_CASE("ProcessInfoDto serializes null resource values when unset")
{
  ProcessInfoDto dto;
  auto document = pidDtoValidDocument();
  document.RemoveMember("uptime_seconds");
  document.RemoveMember("cpu_percent");
  document.RemoveMember("memory_bytes");
  REQUIRE(dto.deserialize(document));

  REQUIRE_FALSE(dto.getUptimeSeconds().has_value());
  REQUIRE_FALSE(dto.getCpuPercent().has_value());
  REQUIRE_FALSE(dto.getMemoryBytes().has_value());

  const std::string json = pidDtoSerialize(dto);
  REQUIRE(json.find("\"cpu_percent\":null") != std::string::npos);
  REQUIRE(json.find("\"uptime_seconds\":null") != std::string::npos);
  REQUIRE(json.find("\"memory_bytes\":null") != std::string::npos);
}

TEST_CASE("ProcessInfoDto reports every missing required field")
{
  const char *const keys[] = { "id",
    "path",
    "auto_restart",
    "restarts",
    "max_restarts",
    "process_id",
    "runs",
    "exit_code",
    "group_name",
    "group_id" };

  for (const char *key : keys) {
    ProcessInfoDto dto;
    auto document = pidDtoValidDocument();
    document.RemoveMember(key);
    INFO("missing key: " << key);
    REQUIRE_FALSE(dto.deserialize(document));
  }
}

TEST_CASE("ProcessInfoDto tolerates missing optional args")
{
  ProcessInfoDto dto;
  auto document = pidDtoValidDocument();
  document.RemoveMember("args");
  REQUIRE(dto.deserialize(document));
  REQUIRE(dto.getArgs().empty());
}

}// namespace lintel
