#ifndef LINTEL_ADMINCONTROLLER_H
#define LINTEL_ADMINCONTROLLER_H

#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/http/service/Controller.h"

namespace lintel {
/**
 * HTTP controller for administrative operations that are not owned by a single
 * feature, currently the reload of the running server's TLS certificate.
 *
 * The endpoint is restricted to the admin group: it changes the certificate the
 * process presents, which is a privilege escalation risk if any user could
 * trigger it (for example to serve a certificate the operator did not intend).
 */
class AdminController : public Controller
{
private:
  Group m_adminGroup;

  ADD_HANDLER_METHOD("/admin/reload-tls", Post, reloadTlsOf);

public:
  /** @param authService authentication service for access control */
  explicit AdminController(const std::shared_ptr<IAuthService> &authService);
};

}// namespace lintel
#endif// LINTEL_ADMINCONTROLLER_H
