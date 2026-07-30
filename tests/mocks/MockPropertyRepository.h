#ifndef CPP_BASE_LIBRARY_MOCKPROPERTYREPOSITORY_H
#define CPP_BASE_LIBRARY_MOCKPROPERTYREPOSITORY_H

#include <catch2/trompeloeil.hpp>

#include "base_library/features/property/repositories/PropertyRepository.h"

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

#endif  // CPP_BASE_LIBRARY_MOCKPROPERTYREPOSITORY_H
