#ifndef CPP_BASE_LIBRARY_SERIALIZATION_H
#define CPP_BASE_LIBRARY_SERIALIZATION_H

#include <exception>
#include <memory>

#include "base_library/core/persistence/postgresql/Serialization.h"
#include "base_library/core/persistence/sqlite3/Serialization.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace db {
template <typename T>
class Serialization {
 public:
  static std::string serialize(
      T type, const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry) {
    switch (connectionEntry->getType()) {
      case ConnectionType::SQLite:
        return sqlite::Serialization<T>::serialize(type);
      case ConnectionType::PostgreSQL:
        return postgresql::Serialization<T>::serialize(type);
      default:
        throw std::runtime_error("not supported connection type");
    }
  }

  static T deserialize(
      const std::string &value,
      const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry) {
    switch (connectionEntry->getType()) {
      case ConnectionType::SQLite:
        return sqlite::Serialization<T>::deserialize(value);
      case ConnectionType::PostgreSQL:
        return postgresql::Serialization<T>::deserialize(value);
      default:
        throw std::runtime_error("not supported connection type");
    }
  }
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_SERIALIZATION_H
