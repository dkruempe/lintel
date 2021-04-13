#include "base_library/persistence/postgresql/Connection.h"

namespace postgresql {
[[nodiscard]] std::shared_ptr<Result>
Connection::execute(const std::string &statement) const {
  return std::make_shared<Result>(PQexec(conn, statement.c_str()));
}

std::shared_ptr<Result>
Connection::executeParameters(const std::string &statement,
                              const db::Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return std::make_shared<Result>(
      PQexecParams(conn, statement.c_str(), params.size(), nullptr, &params[0],
                   &paramLengths[0], nullptr, 0));
}

[[nodiscard]] std::shared_ptr<Result>
Connection::prepareStatement(const std::string &statementName,
                             const std::string &query, int32_t nParams) const {
  return std::make_shared<Result>(
      PQprepare(conn, statementName.c_str(), query.c_str(), nParams, nullptr));
}

[[nodiscard]] std::shared_ptr<Result>
Connection::executePreparedStatement(const std::string &statementName,
                                     const std::string &query, int32_t nParams,
                                     const db::Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return std::make_shared<Result>(PQexecPrepared(conn, statementName.c_str(),
                                                 nParams, &params[0],
                                                 &paramLengths[0], nullptr, 0));
}

[[nodiscard]] std::string Connection::getErrorMessage() const {
  return PQerrorMessage(conn);
}

Connection::Connection(const std::string &connectionInfo)
    : conn(PQconnectdb(connectionInfo.c_str())) {
  if (PQstatus(conn) != CONNECTION_OK) {
    throw db::SQLException("Connection to database failed: " +
                           getErrorMessage());
  }
}

Connection::~Connection() {
  if (conn != nullptr) {
    PQfinish(conn);
    conn = nullptr;
  }
}
} // namespace postgresql