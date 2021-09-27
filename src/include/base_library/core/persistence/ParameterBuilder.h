#ifndef CPP_BASE_LIBRARY_PARAMETERBUILDER_H
#define CPP_BASE_LIBRARY_PARAMETERBUILDER_H

#include <string>
#include <utility>
#include <vector>

#include "base_library/core/persistence/Serialization.h"

namespace db {
class ParameterBuilder {
 private:
  std::vector<std::string> m_parameters;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

 public:
  explicit ParameterBuilder(
      std::shared_ptr<DatabaseConnectionEntry> connectionEntry)
      : m_connectionEntry(std::move(connectionEntry)) {}

  template <typename T>
  ParameterBuilder& add(T t) {
    m_parameters.push_back(
        db::Serialization<T>::serialize(t, m_connectionEntry));
    return *this;
  }

  [[nodiscard]] const std::vector<std::string>& build() const {
    return m_parameters;
  }

  void clear() { m_parameters.clear(); }
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_PARAMETERBUILDER_H
