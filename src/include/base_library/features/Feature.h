#ifndef CPP_BASE_LIBRARY_FEATURE_H
#define CPP_BASE_LIBRARY_FEATURE_H

#include <string>

#include "../../../../external/Hypodermic/include/Hypodermic/Container.h"
#include "../../../../external/Hypodermic/include/Hypodermic/ContainerBuilder.h"
#include "base_library/core/utils/TypeName.h"

class Feature {
 protected:
  std::string_view name;

 public:
  explicit Feature(std::string_view name) : name(name) {}

  ~Feature() = default;

  virtual void registerTypes(Hypodermic::ContainerBuilder &builder) = 0;

  virtual void initialize(std::shared_ptr<Hypodermic::Container> container) = 0;

  std::string_view getName() { return name; }
};

#endif  // CPP_BASE_LIBRARY_FEATURE_H
