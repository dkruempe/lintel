#ifndef CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H

#include <libpq-fe.h>

#include <string>

namespace postgresql {
class Result {
 private:
  PGresult *m_res;

 public:
  explicit Result(PGresult *res);

  ~Result();

  [[nodiscard]] bool isState(ExecStatusType status) const;

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;
};
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
