#ifndef LINTEL_MOCKPROPERTYREPOSITORY_H
#define LINTEL_MOCKPROPERTYREPOSITORY_H

#include <catch2/trompeloeil.hpp>

#include "lintel/features/property/repositories/PropertyRepository.h"

class MockPropertyRepository : public PropertyRepository {
public:
    using PropertyRepository::PropertyRepository;

    MAKE_MOCK0(getDataStorage, DataStorage(), override);
    MAKE_MOCK1(save, void(const std::vector<std::shared_ptr<PropertyBase>> &), override);
    MAKE_MOCK1(save, void(std::shared_ptr<PropertyBase>), override);
    MAKE_MOCK1(deleteOf, void(const std::vector<std::shared_ptr<PropertyBase>> &), override);
    MAKE_MOCK0(awake, std::vector<std::shared_ptr<PropertyBase>>(), override);
    MAKE_MOCK4(allOf, std::vector<std::shared_ptr<PropertyBase>>(const std::string &, const std::string &, const std::string &, const std::string &), override);
};

#endif  // LINTEL_MOCKPROPERTYREPOSITORY_H
