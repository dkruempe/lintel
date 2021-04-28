#ifndef CPP_BASE_LIBRARY_ENTRY_H
#define CPP_BASE_LIBRARY_ENTRY_H

#include <string>
#include <utility>
#include <vector>

namespace db {
/**
 * parameter of an statement or prepared statement
 */
class Parameter {
 private:
  std::string m_value;

  explicit Parameter(std::string value);

 public:
  static Parameter with(const std::string &value);

  [[nodiscard]] const std::string &getValue() const;

  [[nodiscard]] std::size_t getLength() const;

  static int32_t getFormatId();
};

class Parameters {
 private:
  std::vector<Parameter> m_params;

  static std::vector<Parameter> init(const std::vector<std::string> &params);

 public:
  explicit Parameters(const std::vector<std::string> &params);

  explicit Parameters(std::vector<Parameter> params);

  [[nodiscard]] std::vector<const char *> getParameters() const;

  [[nodiscard]] std::vector<int32_t> getParametersLengths() const;

  [[nodiscard]] std::vector<int32_t> getParametersTypes() const;
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_ENTRY_H
