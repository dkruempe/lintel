#ifndef CPP_BASE_LIBRARY_RESULT_H
#define CPP_BASE_LIBRARY_RESULT_H

#include <libpq-fe.h>
#include <string>

namespace db {
class Result {
private:
  PGresult *res;

public:
  explicit Result(PGresult *res);

  ~Result();

  [[nodiscard]] bool isState(ExecStatusType status) const;

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;
};
} // namespace db

#endif // CPP_BASE_LIBRARY_RESULT_H
