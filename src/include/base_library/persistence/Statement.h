#ifndef CPP_BASE_LIBRARY_STATEMENT_H
#define CPP_BASE_LIBRARY_STATEMENT_H

#include "Connection.h"

namespace db {
class Statement {
private:
  const Connection &connection;

  static int32_t initNParams(const std::string &tempStatement);

  std::string initStatement(const std::string &tempStatement);

public:
  explicit Statement(const Connection &connection);

  Result execute(const std::string &query);

  Result execute(const std::string &query, const std::vector<std::string> &params);
};
}

#endif // CPP_BASE_LIBRARY_STATEMENT_H
