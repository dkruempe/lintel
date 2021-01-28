#include "base_library/persistence/postgresql/Result.h"

namespace postgresql {
Result::Result(PGresult *res) : res(res) {}

Result::~Result() {
  if (res != nullptr) {
    PQclear(res);
    res = nullptr;
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
} // namespace postgresql