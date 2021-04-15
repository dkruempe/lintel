#ifndef CPP_BASE_LIBRARY_PROPERTYFEATURE_H
#define CPP_BASE_LIBRARY_PROPERTYFEATURE_H

#include "base_library/features/Feature.h"
#include "base_library/features/property/services/PropertyService.h"

class PropertyFeature : public Feature {
private:
  std::shared_ptr<PropertyService> propertyService;

public:
  PropertyFeature();

  void registerTypes(Hypodermic::ContainerBuilder &builder) override;

  void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif // CPP_BASE_LIBRARY_PROPERTYFEATURE_H
