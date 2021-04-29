
#include "base_library/core/persistence/postgresql/Result.h"

#include <base_library/core/persistence/sqlite3/Result.h>

namespace postgresql {
Result::Result(PGresult *res) : m_res(res) {}

Result::~Result() {
  if (m_res != nullptr) {
    PQclear(m_res);
    m_res = nullptr;
  }
}

[[nodiscard]] bool Result::isState(ExecStatusType status) const {
  return PQresultStatus(m_res) == status;
}

[[nodiscard]] std::string Result::getValue(int row, int attribute) const {
  return PQgetvalue(m_res, row, attribute);
}

[[nodiscard]] int Result::getNumOfAttributes() const { return PQnfields(m_res); }

[[nodiscard]] int Result::getSize() const { return PQntuples(m_res); }
}  // namespace postgresql
