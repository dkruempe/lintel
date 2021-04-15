#include "base_library/core/persistence/Parameter.h"

#include <utility>

namespace db {
Parameter::Parameter(std::string value) : value(std::move(value)) {}

const std::string &Parameter::getValue() const { return value; }

std::size_t Parameter::getLength() const { return value.size(); }

int32_t Parameter::getFormatId() { return 1; }
Parameter Parameter::with(const std::string &value) { return Parameter(value); }

std::vector<Parameter>
Parameters::init(const std::vector<std::string> &params) {
  std::vector<Parameter> temp;
  temp.reserve(params.size());
  for (auto &param : params) {
    temp.push_back(Parameter::with(param));
  }
  return temp;
}

Parameters::Parameters(const std::vector<std::string> &params)
    : params(init(params)) {}

Parameters::Parameters(std::vector<Parameter> params)
    : params(std::move(params)) {}
std::vector<const char *> Parameters::getParameters() const {
  std::vector<const char *> temp;
  for (auto &iter : params) {
    temp.push_back(iter.getValue().c_str());
  }
  return temp;
}
std::vector<int32_t> Parameters::getParametersLengths() const {
  std::vector<int32_t> temp;
  for (auto &iter : params) {
    temp.push_back(iter.getLength());
  }
  return temp;
}
std::vector<int32_t> Parameters::getParametersTypes() const {
  std::vector<int32_t> temp;
  for (auto &iter : params) {
    temp.push_back(iter.getFormatId());
  }
  return temp;
}
} // namespace db