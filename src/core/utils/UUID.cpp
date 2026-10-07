#include "lintel/core/utils/UUID.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string UUID::generate() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
}