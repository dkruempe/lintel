#ifndef PLC_DATASTORAGE_H
#define PLC_DATASTORAGE_H

#include <ostream>
#include <sstream>

#include "base_library/models/PropertyRepositoryType.h"

class DataStorage {
 private:
  PropertyRepositoryType type = PropertyRepositoryType::DEFAULT;
  std::string extraInformation;

 public:
  DataStorage(const PropertyRepositoryType& type,
              const std::string& extraInformation)
      : type(type), extraInformation(extraInformation) {}

  DataStorage() = default;

  [[nodiscard]] const PropertyRepositoryType& getType() const { return type; }
  [[nodiscard]] const std::string& getExtraInformation() const { return extraInformation; }

  friend std::ostream& operator<<(std::ostream& os,
                                  const DataStorage& storage) {
    os << storage.type << "(" << storage.extraInformation << ")";
    return os;
  }

  operator std::string() const {
    std::ostringstream out;
    out << *this;
    return out.str();
  }
};

#endif  // PLC_DATASTORAGE_H
