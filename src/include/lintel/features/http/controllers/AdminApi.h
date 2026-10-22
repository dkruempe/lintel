#ifndef LINTEL_ADMINAPI_H
#define LINTEL_ADMINAPI_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lintel/features/http/provider/ClientProvider.h"

namespace lintel {
/** Result of one reloadable TLS resource, as reported by POST /admin/reload-tls */
struct AdminTlsReloadResult
{
  /** Name of the reloadable resource, e.g. "tls" */
  std::string name;
  /** "ok" or "failed: <reason>" */
  std::string detail;
  /** True when the resource serves the new certificate */
  bool reloaded = false;
};

/** Answer of POST /admin/reload-tls */
struct AdminTlsReloadResponse
{
  /** "reloaded", "failed" or "nothing_to_reload" */
  std::string status;
  /** One entry per reloadable resource, empty when nothing is registered */
  std::vector<AdminTlsReloadResult> results;
};

/** Answer of POST /admin/log-level */
struct AdminLogLevelResponse
{
  /** "changed" when the level was applied, "invalid_level" when it was rejected */
  std::string status;
  /** The level that was requested, empty for a malformed body */
  std::string level;
};

/** API client for the administrative endpoints of the HTTP service */
class AdminApi
{
private:
  std::shared_ptr<Client> m_client;

public:
  /** @param clientProvider provider of the configured HTTP client */
  explicit AdminApi(const std::shared_ptr<ClientProvider> &clientProvider);

  /**
   * Re-read the certificate and private key of the running TLS server.
   * @return the per-resource result, nullopt when no response was received or
   *         the body could not be parsed
   */
  std::optional<AdminTlsReloadResponse> reloadTlsOf();

  /**
   * Change the log level of the running process.
   * @param levelName trace, debug, info, warn, err (or error), critical or off
   * @return the applied or rejected level, nullopt when no response was
   *         received or the body could not be parsed
   */
  std::optional<AdminLogLevelResponse> logLevelOf(const std::string &levelName);
};

}// namespace lintel
#endif// LINTEL_ADMINAPI_H
