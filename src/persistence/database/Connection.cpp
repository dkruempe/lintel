#include "base_library/persistence/database/Connection.h"

namespace db {
[[nodiscard]] Result Connection::execute(const std::string &statement) const {
  return Result(PQexec(conn, statement.c_str()));
}

Result Connection::executeParameters(const std::string &statement,
                                     const Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return Result(PQexecParams(conn, statement.c_str(), params.size(), nullptr,
                             &params[0], &paramLengths[0], nullptr, 0));
}

[[nodiscard]] Result
Connection::prepareStatement(const std::string &statementName,
                             const std::string &query, int32_t nParams) const {
  return Result(
      PQprepare(conn, statementName.c_str(), query.c_str(), nParams, nullptr));
}

[[nodiscard]] Result
Connection::executePreparedStatement(const std::string &statementName,
                                     const std::string &query, int32_t nParams,
                                     const Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return Result(PQexecPrepared(conn, statementName.c_str(), nParams, &params[0],
                               &paramLengths[0], nullptr, 0));
}

[[nodiscard]] std::string Connection::getErrorMessage() const {
  return PQerrorMessage(conn);
}

Connection::Connection(const std::string &connectionInfo)
    : conn(PQconnectdb(connectionInfo.c_str())) {
  if (PQstatus(conn) != CONNECTION_OK) {
    throw SQLException("Connection to database failed: " + getErrorMessage());
  }
}

Connection::~Connection() {
  if (conn != nullptr) {
    PQfinish(conn);
    conn = nullptr;
  }
}
} // namespace db