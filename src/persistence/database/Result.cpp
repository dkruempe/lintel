#include "base_library/persistence/database/Result.h"

namespace db {
Result::Result(PGresult *res) : res(res) {}

Result::~Result() {
  if (res != nullptr) {
    PQclear(res);
  }
}

[[nodiscard]] bool Result::isState(ExecStatusType status) const {
  return PQresultStatus(res) == status;
}

[[nodiscard]] std::string Result::getValue(int row, int attribute) const {
  return PQgetvalue(res, row, attribute);
}

[[nodiscard]] int Result::getNumOfAttributes() const { return PQnfields(res); }

[[nodiscard]] int Result::getSize() const { return PQntuples(res); }
} // namespace db