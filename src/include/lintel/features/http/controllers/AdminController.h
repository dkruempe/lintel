#ifndef LINTEL_ADMINCONTROLLER_H
#define LINTEL_ADMINCONTROLLER_H

#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/**
 * HTTP controller for administrative operations that are not owned by a single
 * feature, currently the reload of the running server's TLS certificate and the
 * runtime log level.
 *
 * The endpoints are restricted to an admin group: they change how the process
 * behaves at runtime, which is a privilege escalation risk if any user could
 * trigger them (for example to log at trace and record credentials, or to serve
 * a certificate the operator did not intend).
 */
class AdminController : public Controller
{
private:
  Group m_adminGroup;
  Group m_adminLogGroup;

  ADD_HANDLER_METHOD("/admin/reload-tls", Post, reloadTlsOf);

  ADD_HANDLER_METHOD("/admin/log-level", Post, logLevelOf);

public:
  /** @param authService authentication service for access control */
  explicit AdminController(const std::shared_ptr<IAuthService> &authService);
};

}// namespace lintel
#endif// LINTEL_ADMINCONTROLLER_H
