#ifndef CPP_BASE_LIBRARY_RESULT_H
#define CPP_BASE_LIBRARY_RESULT_H

#include "base_library/persistence/Connection.h"
#include "base_library/persistence/postgresql/Result.h"
#include <string>

namespace db {
class Result {
private:
  std::shared_ptr<postgresql::Result> result = nullptr;

public:
  Result() = default;

  ~Result();

  explicit Result(std::shared_ptr<postgresql::Result> result);

  [[nodiscard]] std::string getValue(int row, int attribute) const;

  [[nodiscard]] int getNumOfAttributes() const;

  [[nodiscard]] int getSize() const;
};
} // namespace db

#endif // CPP_BASE_LIBRARY_RESULT_H
