#include <catch2/catch_all.hpp>
#include <catch2/trompeloeil.hpp>

#include <memory>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/DataStorage.h"
#include "lintel/core/models/Property.h"
#include "lintel/core/models/PropertyBase.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/property/configuration/PropertyRepositoryComponent.h"
#include "lintel/features/property/configuration/PropertyRepositoryEntry.h"
#include "lintel/features/property/exceptions/PropertyNoRuntimeChangeSupported.h"
#include "lintel/features/property/exceptions/PropertyNotFoundException.h"
#include "lintel/features/property/factories/PropertyFactory.h"
#include "lintel/features/property/services/PropertyService.h"

#include "../mocks/MockPropertyRepository.h"

namespace lintel {
namespace {

  using PropertyVector = std::vector<std::shared_ptr<PropertyBase>>;

  std::shared_ptr<Configuration> propSvcConfig(const std::vector<std::shared_ptr<Entry>> &entries)
  {
    auto environment = std::make_shared<EnvironmentConfiguration>();
    environment->overrides(
      EnvironmentConfiguration::ConfigDirectory, "/nonexistent/lintel-propsvc-repository-test-config");
    auto configuration =
      std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, std::move(environment));
    configuration->setEntries(entries);
    return configuration;
  }

  std::shared_ptr<PropertyRepositoryEntry> propSvcEntry(PropertyRepositoryType type, bool mutableFlag, bool shadowFlag)
  {
    return std::make_shared<PropertyRepositoryEntry>(
      type_name<PropertyRepositoryComponent>(), type, mutableFlag, shadowFlag);
  }

  std::shared_ptr<PropertyBase> propSvcProperty(const std::string &name,
    const std::string &instance = "instance",
    const std::string &className = "klass",
    const std::string &processName = "process")
  {
    return PropertyFactory::Create(name, instance, className, processName, "int32_t", "1", "", true);
  }

}// namespace

TEST_CASE("PropertyService: onAwake loads every enabled repository and allOf filters by coordinates")
{
  auto configuration = propSvcConfig({ propSvcEntry(PropertyRepositoryType::FILE_REPOSITORY, false, false),
    propSvcEntry(PropertyRepositoryType::SHM_REPOSITORY, true, false) });
  auto fileRepository =
    std::make_shared<MockPropertyRepository>(PropertyRepositoryType::FILE_REPOSITORY, configuration);
  auto sharedMemoryRepository =
    std::make_shared<MockPropertyRepository>(PropertyRepositoryType::SHM_REPOSITORY, configuration);

  ALLOW_CALL(*fileRepository, awake()).LR_RETURN(PropertyVector{ propSvcProperty("fileProp") });
  ALLOW_CALL(*sharedMemoryRepository, awake()).LR_RETURN(PropertyVector{ propSvcProperty("shmProp") });

  PropertyService service({ fileRepository, sharedMemoryRepository }, {});
  service.onAwake();

  REQUIRE(service.allOf().size() == 2);
  REQUIRE(service.allOf("process").size() == 2);
  REQUIRE(service.allOf("process", "klass").size() == 2);
  REQUIRE(service.allOf("process", "klass", "instance").size() == 2);
  // note: the single-argument allOf() overload returns the whole local map
  REQUIRE(service.allOf("noprocess").size() == 2);
  REQUIRE(service.allOf("process", "noklass").empty());
  REQUIRE(service.allOf("process", "klass", "noinstance").empty());
}

TEST_CASE("PropertyService: allOf delegates to the shadow repository when one is configured")
{
  auto configuration = propSvcConfig({ propSvcEntry(PropertyRepositoryType::SHM_REPOSITORY, true, true) });
  auto shadowRepository =
    std::make_shared<MockPropertyRepository>(PropertyRepositoryType::SHM_REPOSITORY, configuration);

  ALLOW_CALL(*shadowRepository, awake()).LR_RETURN(PropertyVector{});
  ALLOW_CALL(*shadowRepository, save(ANY(std::vector<std::shared_ptr<PropertyBase>>)));
  ALLOW_CALL(*shadowRepository, allOf(trompeloeil::_)).LR_RETURN(PropertyVector{ propSvcProperty("shadowed") });

  PropertyService service({ shadowRepository }, {});
  service.onAwake();

  REQUIRE(service.allOf().size() == 1);
  REQUIRE(service.allOf("process").size() == 1);
  REQUIRE(service.allOf("process", "klass").size() == 1);
  REQUIRE(service.allOf("process", "klass", "instance").size() == 1);
  REQUIRE(service.allOf().front()->getName() == "shadowed");
}

TEST_CASE("PropertyService: getOrCreate, changeValueOf and changeStringValueOf persist through mutable repositories")
{
  auto configuration = propSvcConfig({ propSvcEntry(PropertyRepositoryType::SHM_REPOSITORY, true, false) });
  auto repository = std::make_shared<MockPropertyRepository>(PropertyRepositoryType::SHM_REPOSITORY, configuration);

  ALLOW_CALL(*repository, awake()).LR_RETURN(PropertyVector{});
  ALLOW_CALL(*repository, getDataStorage()).LR_RETURN(DataStorage());
  ALLOW_CALL(*repository, save(ANY(std::shared_ptr<PropertyBase>)));

  PropertyService service({ repository }, {});
  auto property = service.getOrCreate<int32_t>("counter", "instance", "klass", "process", "", true, 0);
  REQUIRE(property->getValue() == 0);

  service.changeValueOf<int32_t>(property, 5);
  REQUIRE(property->getValue() == 5);

  service.changeStringValueOf(property, "7");
  REQUIRE(property->getValue() == 7);

  // unchanged value is a no-op
  service.changeStringValueOf(property, "7");
  REQUIRE(property->getValue() == 7);
}

TEST_CASE("PropertyService: get throws for an unknown property and runtime changes are validated")
{
  auto configuration = propSvcConfig({ propSvcEntry(PropertyRepositoryType::SHM_REPOSITORY, true, false) });
  auto repository = std::make_shared<MockPropertyRepository>(PropertyRepositoryType::SHM_REPOSITORY, configuration);

  ALLOW_CALL(*repository, awake()).LR_RETURN(PropertyVector{});
  ALLOW_CALL(*repository, getDataStorage()).LR_RETURN(DataStorage());
  ALLOW_CALL(*repository, save(ANY(std::shared_ptr<PropertyBase>)));

  PropertyService service({ repository }, {});
  REQUIRE_THROWS_AS(service.get("missing", "instance", "klass", "process"), PropertyNotFoundException);

  auto readOnly = service.getOrCreate<int32_t>("readOnly", "instance", "klass", "process", "", false, 0);
  REQUIRE_THROWS_AS(service.changeValueOf<int32_t>(readOnly, 1), PropertyNoRuntimeChangeSupported);
  REQUIRE_THROWS_AS(service.changeStringValueOf(readOnly, "1"), PropertyNoRuntimeChangeSupported);
}

TEST_CASE("PropertyService: get returns an existing property")
{
  PropertyService service({}, {});
  service.getOrCreate<int32_t>("present", "instance", "klass", "process", "", true, 3);

  auto &property = service.get("present", "instance", "klass", "process");
  REQUIRE(property->toString() == "3");
}

}// namespace lintel
