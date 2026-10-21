#include <httplib.h>

#include <optional>
#include <string>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/http/controllers/AdminApi.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

namespace lintel {
namespace {
  /** @return {"level":"<levelName>"} */
  std::string logLevelBodyOf(const std::string &levelName)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("level");
    writer.String(levelName.c_str(), static_cast<rapidjson::SizeType>(levelName.size()));
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }

  /**
   * @param result a completed request
   * @return the response body, nullopt when no response was received
   * @throws HttpUnauthorizedException if the token was missing or rejected
   *
   * Unlike the read endpoints of the other API clients this does not require
   * status 200: the admin routes answer 409 "nothing_to_reload", 400
   * "invalid_level" and 500 "failed" with a body that the caller reports.
   */
  std::optional<std::string> bodyOf(const httplib::Result &result)
  {
    if (!HttpClientHelper::hasResponse(result)) { return std::nullopt; }
    if (HttpStatusCodes(result->status) == HttpStatusCodes::Unauthorized) { throw HttpUnauthorizedException(); }
    return result->body;
  }
}// namespace

AdminApi::AdminApi(const std::shared_ptr<ClientProvider> &clientProvider) : m_client(clientProvider->provide()) {}

std::optional<AdminTlsReloadResponse> AdminApi::reloadTlsOf()
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->post("/admin/reload-tls", headers, "", "application/json");
  const std::optional<std::string> body = bodyOf(result);
  if (!body.has_value()) { return std::nullopt; }
  rapidjson::Document document;
  document.Parse(body->c_str());
  if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
  AdminTlsReloadResponse response;
  if (!json::getString(document, "status", response.status)) { return std::nullopt; }
  const rapidjson::Value *results = json::member(document, "results");
  if (results == nullptr || !results->IsObject()) { return std::nullopt; }
  for (auto iter = results->MemberBegin(); iter != results->MemberEnd(); ++iter) {
    // a malformed entry is skipped instead of failing the whole answer: the
    // status still tells the caller whether the reload as a whole succeeded
    if (!iter->name.IsString() || !iter->value.IsString()) { continue; }
    AdminTlsReloadResult reloadResult;
    reloadResult.name = std::string(iter->name.GetString(), iter->name.GetStringLength());
    reloadResult.detail = std::string(iter->value.GetString(), iter->value.GetStringLength());
    reloadResult.reloaded = reloadResult.detail == "ok";
    response.results.push_back(std::move(reloadResult));
  }
  return response;
}

std::optional<AdminLogLevelResponse> AdminApi::logLevelOf(const std::string &levelName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result =
    m_client->post("/admin/log-level", headers, logLevelBodyOf(levelName), "application/json");
  const std::optional<std::string> body = bodyOf(result);
  if (!body.has_value()) { return std::nullopt; }
  rapidjson::Document document;
  document.Parse(body->c_str());
  if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
  AdminLogLevelResponse response;
  if (!json::getString(document, "status", response.status)) { return std::nullopt; }
  // the controller leaves the level empty for a malformed body, so a missing
  // member is not an error here
  json::getString(document, "level", response.level);
  return response;
}
}// namespace lintel
