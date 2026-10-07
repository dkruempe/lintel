#ifndef LINTEL_PROPERTYFEATURE_H
#define LINTEL_PROPERTYFEATURE_H

#include "lintel/features/Feature.h"
#include "lintel/features/property/services/PropertyService.h"

/** Feature that registers and initializes the property management system */
class PropertyFeature : public Feature<Features::Value> {
private:
    std::shared_ptr<PropertyService> m_propertyService;

public:
    /** @param features parent features container */
    PropertyFeature(std::shared_ptr<Features> features);

    /** Register property types in the dependency injection container */
    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    /** Initialize the property service from the container */
    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // LINTEL_PROPERTYFEATURE_H
