#ifndef CPP_BASE_LIBRARY_ARGUMENTS_H
#define CPP_BASE_LIBRARY_ARGUMENTS_H

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/persistence/Serialization.h"
#include "base_library/core/services/StringifyService.h"

namespace db {
class Argument {
 private:
  std::string m_value;
  std::string m_name;
  db::ConnectionType m_connectionType;

 public:
  Argument(std::string value, std::string name);
  [[nodiscard]] const std::string &getValue() const;
  [[nodiscard]] const std::string &getName() const;
  void setConnectionType(ConnectionType connectionType);

  template <typename TYPE>
  TYPE getValue() const {
    switch (m_connectionType) {
      case ConnectionType::PostgreSQL:
        return postgresql::Serialization<TYPE>::deserialize(m_value);
      case ConnectionType::UNDEFINED:
        return StringifyService<TYPE>::deserializeFromString(m_value);
    }
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const db::Argument &argument);
};
class Arguments {
 private:
  // members
  std::vector<Argument> m_arguments;
  std::map<std::string, Argument> m_argumentsMap;
  ConnectionType m_connectionType;

  // iterator classes
  class Iterator
      : public std::iterator<std::random_access_iterator_tag, Argument,
                             std::ptrdiff_t, Argument *, Argument &> {
   private:
    Arguments &m_arguments;
    std::size_t m_pos;

   public:
    explicit Iterator(Arguments &arguments, bool end = false)
        : m_arguments(arguments),
          m_pos(end ? static_cast<std::size_t>(m_arguments.size()) : 0) {}

    Iterator &operator++() {
      ++m_pos;
      return *this;
    }
    Iterator operator++(int) {
      ++m_pos;
      return *this;
    }
    bool operator==(Iterator other) const { return m_pos == other.m_pos; }
    bool operator!=(Iterator other) const { return !(*this == other); }
    reference operator*() { return m_arguments.of(m_pos); }
  };

  class ConstIterator
      : public std::iterator<std::random_access_iterator_tag, Argument,
                             std::ptrdiff_t, const Argument *,
                             const Argument &> {
   private:
    const Arguments &m_arguments;
    std::size_t m_pos;

   public:
    explicit ConstIterator(const Arguments &arguments, bool end = false)
        : m_arguments(arguments),
          m_pos(end ? static_cast<std::size_t>(m_arguments.size()) : 0) {}

    ConstIterator &operator++() {
      ++m_pos;
      return *this;
    }
    ConstIterator operator++(int) {
      ++m_pos;
      return *this;
    }
    bool operator==(ConstIterator other) const { return m_pos == other.m_pos; }
    bool operator!=(ConstIterator other) const { return !(*this == other); }
    reference operator*() const { return m_arguments.of(m_pos); }
  };

 public:
  // constructor
  explicit Arguments(ConnectionType connectionType);
  // helper methods
  void add(Argument &argument);
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] bool empty() const;
  Argument &of(std::size_t iterator);
  [[nodiscard]] const Argument &of(std::size_t iterator) const;
  [[nodiscard]] const Argument &of(const std::string &name) const;

  // iterators
  Iterator begin() { return Iterator(*this); }
  Iterator end() { return Iterator(*this, true); }
  [[nodiscard]] ConstIterator begin() const { return ConstIterator(*this); }
  [[nodiscard]] ConstIterator end() const { return ConstIterator(*this, true); }
};
};  // namespace db

#endif  // CPP_BASE_LIBRARY_ARGUMENTS_H
