#include "lintel/features/http/controllers/AdminController.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>
#include <utility>
#include <vector>

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
}// namespace

AdminController::AdminController(const std::shared_ptr<IAuthService> &authService)
  : Controller(authService), m_adminGroup("Admin-Tls", {}, true)
{
  add(m_adminGroup);
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
}// namespace lintel
