#include "lintel/features/http/controllers/AdminController.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/http/service/TlsReloadRegistry.h"

namespace lintel {
namespace {
  /**
   * @return {"status":"<status>","results":{"<name>":"ok|failed: <reason>"}}
   *
   * rapidjson writes the body so that a failure reason coming from an exception
   * message (quotes, backslashes, control characters) cannot break the JSON.
   */
  std::string reloadBody(const std::vector<TlsReloadRegistry::ReloadResult> &results, const char *status)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String(status);
    writer.Key("results");
    writer.StartObject();
    for (const auto &result : results) {
      writer.Key(result.m_name.c_str(), static_cast<rapidjson::SizeType>(result.m_name.size()));
      if (result.m_reloaded) {
        writer.String("ok");
      } else {
        const std::string detail = result.m_detail.empty() ? "failed" : "failed: " + result.m_detail;
        writer.String(detail.c_str(), static_cast<rapidjson::SizeType>(detail.size()));
      }
    }
    writer.EndObject();
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }

  /** @return {"status":"<status>","level":"<level>"} */
  std::string logLevelBody(const char *status, const std::string &levelName)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String(status);
    writer.Key("level");
    writer.String(levelName.c_str(), static_cast<rapidjson::SizeType>(levelName.size()));
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }
}// namespace

AdminController::AdminController(const std::shared_ptr<IAuthService> &authService)
  : Controller(authService), m_adminGroup("Admin-Tls", {}, true), m_adminLogGroup("Admin-Log", {}, true)
{
  add(m_adminGroup);
  add(m_adminLogGroup);
}

void AdminController::reloadTlsOfPost(const httplib::Request &,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value() || !user->m_user.has(m_adminGroup)) {
    // Same answer for "no token" and "not an admin": telling them apart would
    // confirm to an anonymous caller that the endpoint exists.
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  const std::vector<TlsReloadRegistry::ReloadResult> results = TlsReloadRegistry::instance().reload();
  if (results.empty()) {
    // No TLS server registered a handler: the process either serves clear text
    // or is not the process that owns the server. There is nothing to reload.
    response.status = HttpStatusCodes::Conflict;
    response.set_content(reloadBody(results, "nothing_to_reload"), contentType.getName());
    return;
  }
  bool reloaded = true;
  for (const auto &result : results) { reloaded = reloaded && result.m_reloaded; }
  response.status = reloaded ? HttpStatusCodes::OK : HttpStatusCodes::InternalServerError;
  response.set_content(reloadBody(results, reloaded ? "reloaded" : "failed"), contentType.getName());
}

void AdminController::logLevelOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!user.has_value() || !user->m_user.has(m_adminLogGroup)) {
    // Same answer for "no token" and "not an admin": telling them apart would
    // confirm to an anonymous caller that the endpoint exists.
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return;
  }
  rapidjson::Document body;
  body.Parse(request.body.c_str());
  // json::getString reports a missing or mistyped member instead of asserting on it,
  // so a client that sends {"level": 42} gets a 400 instead of undefined behaviour
  std::string levelName;
  if (body.HasParseError() || !json::getString(body, "level", levelName)) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content(logLevelBody("invalid_level", ""), contentType.getName());
    return;
  }
  const std::optional<LogLevel> level = LoggerService::logLevelFrom(levelName);
  if (!level.has_value()) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content(logLevelBody("invalid_level", levelName), contentType.getName());
    return;
  }
  // The new level takes effect immediately for every log call in this process; the
  // configured level of the bootstrap configuration is not rewritten, so a restart
  // goes back to what the operator deployed.
  LoggerService::get().setLevel(level.value());
  LOG_INFO("log level changed to {}", levelName);
  response.status = HttpStatusCodes::OK;
  response.set_content(logLevelBody("changed", levelName), contentType.getName());
}
}// namespace lintel
