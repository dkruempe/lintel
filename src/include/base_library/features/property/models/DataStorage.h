#ifndef PLC_DATASTORAGE_H
#define PLC_DATASTORAGE_H

#include <ostream>
#include <sstream>
#include <utility>

#include "PropertyRepositoryType.h"

class DataStorage {
 private:
  PropertyRepositoryType m_type = PropertyRepositoryType::DEFAULT;
  std::string m_extraInformation;

 public:
  DataStorage(const PropertyRepositoryType &type, std::string extraInformation)
      : m_type(type), m_extraInformation(std::move(extraInformation)) {}

  DataStorage() = default;

  [[nodiscard]] const PropertyRepositoryType &getType() const { return m_type; }
  [[nodiscard]] const std::string &getExtraInformation() const {
    return m_extraInformation;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const DataStorage &storage) {
    os << storage.m_type << "(" << storage.m_extraInformation << ")";
    return os;
  }

  operator std::string() const {
    std::ostringstream out;
    out << *this;
    return out.str();
  }
};

#endif  // PLC_DATASTORAGE_H
