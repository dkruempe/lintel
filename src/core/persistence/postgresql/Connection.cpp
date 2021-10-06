#include "base_library/core/persistence/postgresql/Connection.h"

namespace postgresql {
[[nodiscard]] std::shared_ptr<Result> Connection::execute(
    const std::string &statement) const {
  return std::make_shared<Result>(PQexec(m_conn, statement.c_str()));
}

std::shared_ptr<Result> Connection::executeParameters(
    const std::string &statement, const db::Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return std::make_shared<Result>(
      PQexecParams(m_conn, statement.c_str(), static_cast<int>(params.size()),
                   nullptr, &params[0], &paramLengths[0], nullptr, 0));
}

[[nodiscard]] std::shared_ptr<Result> Connection::prepareStatement(
    const std::string &statementName, const std::string &query,
    int32_t nParams) const {
  return std::make_shared<Result>(PQprepare(m_conn, statementName.c_str(),
                                            query.c_str(), nParams, nullptr));
}

[[nodiscard]] std::shared_ptr<Result> Connection::executePreparedStatement(
    const std::string &statementName, const std::string &query, int32_t nParams,
    const db::Parameters &parameters) const {
  const std::vector<const char *> &params = parameters.getParameters();
  const std::vector<int32_t> &paramLengths = parameters.getParametersLengths();
  return std::make_shared<Result>(PQexecPrepared(m_conn, statementName.c_str(),
                                                 nParams, &params[0],
                                                 &paramLengths[0], nullptr, 0));
}

[[nodiscard]] std::string Connection::getErrorMessage() const {
  return PQerrorMessage(m_conn);
}

Connection::Connection(const std::string &connectionInfo)
    : m_conn(PQconnectdb(connectionInfo.c_str())) {
  if (PQstatus(m_conn) != CONNECTION_OK) {
    throw db::SQLException("Connection to database failed: " +
                           getErrorMessage());
  }
}

Connection::Connection(
    const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
    : m_conn(nullptr) {
  std::string connInfo;
  if (!connectionEntry->getUserName().empty()) {
    connInfo += "user=";
    connInfo += connectionEntry->getUserName();
  }
  if (!connectionEntry->getPassword().empty()) {
    connInfo += " ";
    connInfo += "password=";
    connInfo += connectionEntry->getPassword();
  }
  if (!connectionEntry->getConnection().empty()) {
    connInfo += " ";
    connInfo += "hostaddr=";
    connInfo += connectionEntry->getConnection();
  }
  if (!connectionEntry->getDatabaseName().empty()) {
    connInfo += " ";
    connInfo += "dbname=";
    connInfo += connectionEntry->getDatabaseName();
  }
  if (connectionEntry->getPort() > 0) {
    connInfo += " ";
    connInfo += "port=";
    connInfo += std::to_string(connectionEntry->getPort());
  }
  m_conn = PQconnectdb(connInfo.c_str());
  if (PQstatus(m_conn) != CONNECTION_OK) {
    throw db::SQLException("Connection to database failed: " +
                           getErrorMessage());
  }
}

Connection::~Connection() {
  if (m_conn != nullptr) {
    PQfinish(m_conn);
    m_conn = nullptr;
  }
}
}  // namespace postgresql