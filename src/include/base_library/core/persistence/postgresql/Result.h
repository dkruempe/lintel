#ifndef CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H

#include <libpq-fe.h>

#include <string>

#include "base_library/core/persistence/Arguments.h"

namespace postgresql {
class Result {
 private:
  PGresult *m_res;
  std::vector<db::Arguments> m_arguments;

 public:
  explicit Result(PGresult *res);

  ~Result();

  [[nodiscard]] bool isState(ExecStatusType status) const;

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;

  [[nodiscard]] db::Arguments &of(std::size_t pos);
};
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
