#include "base_library/persistence/Result.h"

#include <utility>

namespace db {
std::string Result::getValue(int row, int attribute) const {
  if (result != nullptr) {
    return result->getValue(row, attribute);
  }
  if (resultSQLite != nullptr) {
    return resultSQLite->getValue(row, attribute);
  }
  return std::string();
}

int Result::getNumOfAttributes() const {
  if (result != nullptr) {
    return result->getNumOfAttributes();
  }
  if (resultSQLite != nullptr) {
    return resultSQLite->getNumOfAttributes();
  }
  return 0;
}

int Result::getSize() const {
  if (result != nullptr) {
    return result->getSize();
  }
  if (resultSQLite != nullptr) {
    return resultSQLite->getSize();
  }
  return 0;
}

Result::Result(std::shared_ptr<postgresql::Result> result)
    : result(std::move(result)), resultSQLite(nullptr) {}

Result::Result(std::shared_ptr<sqlite::Result> result)
    : result(nullptr), resultSQLite(std::move(result)) {}

Result::~Result() = default;
} // namespace db