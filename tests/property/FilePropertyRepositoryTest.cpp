#include <catch2/catch_all.hpp>

#include <lintel/core/utils/TypeName.h>
#include <lintel/features/base/configuration/Configuration.h>
#include <lintel/features/base/configuration/ConfigurationException.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/models/ProcessName.h>
#include <lintel/features/property/configuration/PropertyComponent.h>
#include <lintel/features/property/configuration/PropertyRepositoryComponent.h>
#include <lintel/features/property/configuration/PropertyRepositoryEntry.h>
#include <lintel/features/property/models/Property.h>
#include <lintel/features/property/models/PropertyRepositoryType.h>
#include <lintel/features/property/repositories/FilePropertyRepository.h>


#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace {

// `fpr` prefix: unity builds merge several test files into one translation unit
// and share one anonymous namespace, so a generic `configuration` or `repository`
// would collide (and shadow local variables, Clang -Wshadow).
constexpr char fprBootstrapName[] = "fpr_bootstrap";

std::string fprConfigDirectory()
{
  static int counter = 0;
  return (std::filesystem::temp_directory_path()
          / ("file_property_repository_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter)))
    .string();
}

/** @return the XML configuration used by every test case in this file */
std::string fprBootstrapXml()
{
  return R"(<Configuration>
    <PropertyRepositories>
        <PropertyRepository type="FILE_REPOSITORY" mutable="false" shadow="false"/>
    </PropertyRepositories>
    <Properties>
        <Property name="fprInt" type="int32_t" value="42" process="fprProcess" class="FprService" instance="__DEFAULT"/>
        <Property name="fprString" type="std::string" value="configured" process="fprProcess" class="FprService" instance="__DEFAULT"/>
        <Property name="fprForeign" type="int32_t" value="7" process="otherProcess" class="FprService" instance="__DEFAULT"/>
    </Properties>
</Configuration>)";
}

/** @return the whole XML file of a configuration directory, read back */
std::string fprReadBootstrap(const std::string &directory)
{
  const std::filesystem::path file = std::filesystem::path(directory) / (std::string(fprBootstrapName) + ".xml");
  std::ifstream stream(file);
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

void fprWriteBootstrap(const std::string &directory, const std::string &content)
{
  const std::filesystem::path file = std::filesystem::path(directory) / (std::string(fprBootstrapName) + ".xml");
  // the stream has to be closed before the file is read back
  {
    std::ofstream stream(file);
    stream << content;
  }
}

/** Removes the temporary configuration directory of one test case */
struct FprConfigDirectoryGuard
{
  std::string m_directory;

  explicit FprConfigDirectoryGuard(std::string directory) : m_directory(std::move(directory))
  {
    std::error_code error;
    std::filesystem::create_directories(m_directory, error);
    REQUIRE(!error);
  }

  ~FprConfigDirectoryGuard()
  {
    std::error_code error;
    std::filesystem::remove_all(m_directory, error);
  }

  FprConfigDirectoryGuard(const FprConfigDirectoryGuard &) = delete;
  FprConfigDirectoryGuard &operator=(const FprConfigDirectoryGuard &) = delete;
};

std::shared_ptr<Configuration> fprConfiguration(const std::shared_ptr<EnvironmentConfiguration> &envConfig)
{
  return std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{ std::make_shared<PropertyComponent>(),
                                           std::make_shared<PropertyRepositoryComponent>() },
    envConfig);
}

}// namespace

TEST_CASE("FilePropertyRepository::awake returns only the properties of its own process", "[file_property_repository]")
{
  const std::string directory = fprConfigDirectory();
  FprConfigDirectoryGuard guard(directory);
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", directory };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", fprBootstrapName };
  fprWriteBootstrap(directory, fprBootstrapXml());

  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  auto configuration = fprConfiguration(envConfig);
  auto processName = std::make_shared<ProcessName>("fprProcess");
  FilePropertyRepository repository(configuration, processName);

  const auto properties = repository.awake();
  REQUIRE(properties.size() == 2);
  REQUIRE(properties[0]->getName() == "fprInt");
  REQUIRE(properties[0]->getProcessName() == "fprProcess");
  REQUIRE(properties[0]->toString() == "42");
  REQUIRE(properties[1]->getName() == "fprString");
  REQUIRE(properties[1]->toString() == "configured");
}

TEST_CASE("FilePropertyRepository::save leaves the configuration file untouched", "[file_property_repository]")
{
  const std::string directory = fprConfigDirectory();
  FprConfigDirectoryGuard guard(directory);
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", directory };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", fprBootstrapName };
  fprWriteBootstrap(directory, fprBootstrapXml());
  const std::string before = fprReadBootstrap(directory);

  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  auto configuration = fprConfiguration(envConfig);
  auto processName = std::make_shared<ProcessName>("fprProcess");
  FilePropertyRepository repository(configuration, processName);

  // a changed property value: writing it back would be the "obvious" implementation
  auto property = std::make_shared<Property<int32_t>>(
    "fprInt", "__DEFAULT", "FprService", "fprProcess", 4711, "changed at runtime", true);

  REQUIRE_NOTHROW(repository.save(property));
  REQUIRE(fprReadBootstrap(directory) == before);

  REQUIRE_NOTHROW(repository.save(std::vector<std::shared_ptr<PropertyBase>>{ property }));
  REQUIRE(fprReadBootstrap(directory) == before);

  // a second read of the configuration still yields the configured default, not the runtime value
  auto reread = fprConfiguration(std::make_shared<EnvironmentConfiguration>());
  FilePropertyRepository reloaded(reread, std::make_shared<ProcessName>("fprProcess"));
  const auto reloadedProperties = reloaded.awake();
  REQUIRE(reloadedProperties.size() == 2);
  REQUIRE(reloadedProperties[0]->toString() == "42");
}

TEST_CASE("FilePropertyRepository::deleteOf leaves the configuration file untouched", "[file_property_repository]")
{
  const std::string directory = fprConfigDirectory();
  FprConfigDirectoryGuard guard(directory);
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", directory };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", fprBootstrapName };
  fprWriteBootstrap(directory, fprBootstrapXml());
  const std::string before = fprReadBootstrap(directory);

  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  auto configuration = fprConfiguration(envConfig);
  auto processName = std::make_shared<ProcessName>("fprProcess");
  FilePropertyRepository repository(configuration, processName);

  const auto properties = repository.awake();
  REQUIRE(properties.size() == 2);

  // deleteOf() is a no-op of the base class; the file must survive it
  REQUIRE_NOTHROW(repository.deleteOf(properties));
  REQUIRE(fprReadBootstrap(directory) == before);
  REQUIRE(repository.awake().size() == 2);
}

TEST_CASE("FilePropertyRepository is enabled but never mutable", "[file_property_repository]")
{
  const std::string directory = fprConfigDirectory();
  FprConfigDirectoryGuard guard(directory);
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", directory };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", fprBootstrapName };
  fprWriteBootstrap(directory, fprBootstrapXml());

  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  auto configuration = fprConfiguration(envConfig);
  FilePropertyRepository repository(configuration, std::make_shared<ProcessName>("fprProcess"));

  REQUIRE(repository.getType() == PropertyRepositoryType::FILE_REPOSITORY);
  REQUIRE(repository.isEnabled());
  // PropertyService filters on isMutable() before it ever calls save(), which is why the no-op
  // implementations of save()/deleteOf() are unreachable in production
  REQUIRE_FALSE(repository.isMutable());
  REQUIRE_FALSE(repository.isShadow());
  REQUIRE(repository.getDataStorage().getType() == PropertyRepositoryType::FILE_REPOSITORY);
  REQUIRE(repository.getDataStorage().getExtraInformation().empty());
}

TEST_CASE("PropertyRepositoryComponent rejects a mutable FILE_REPOSITORY", "[file_property_repository]")
{
  PropertyRepositoryComponent component;

  const std::string xml = R"(<PropertyRepositories>
        <PropertyRepository type="FILE_REPOSITORY" mutable="true" shadow="false"/>
    </PropertyRepositories>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("PropertyRepositoryComponent rejects a shadow FILE_REPOSITORY", "[file_property_repository]")
{
  PropertyRepositoryComponent component;

  const std::string xml = R"(<PropertyRepositories>
        <PropertyRepository type="FILE_REPOSITORY" mutable="false" shadow="true"/>
    </PropertyRepositories>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("PropertyRepositoryComponent accepts an immutable FILE_REPOSITORY", "[file_property_repository]")
{
  PropertyRepositoryComponent component;

  const std::string xml = R"(<PropertyRepositories>
        <PropertyRepository type="FILE_REPOSITORY" mutable="false" shadow="false"/>
    </PropertyRepositories>)";

  const auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 1);
  const auto entry = std::static_pointer_cast<PropertyRepositoryEntry>(entries[0]);
  REQUIRE(entry->getType() == PropertyRepositoryType::FILE_REPOSITORY);
  REQUIRE_FALSE(entry->isMutable());
  REQUIRE_FALSE(entry->isShadow());
  REQUIRE(entry->getConfigurationParserComponent() == type_name<PropertyRepositoryComponent>());
}

TEST_CASE("PropertyRepositoryComponent rejects an undefined repository type", "[file_property_repository]")
{
  PropertyRepositoryComponent component;

  const std::string xml = R"(<PropertyRepositories>
        <PropertyRepository type="DEFAULT" mutable="false" shadow="false"/>
    </PropertyRepositories>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("PropertyRepositoryComponent rejects a shadow repository that is not mutable", "[file_property_repository]")
{
  PropertyRepositoryComponent component;

  const std::string xml = R"(<PropertyRepositories>
        <PropertyRepository type="DATABASE_REPOSITORY" mutable="false" shadow="true"/>
    </PropertyRepositories>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
}

TEST_CASE("FilePropertyRepository is disabled when no FILE_REPOSITORY entry is configured",
  "[file_property_repository]")
{
  const std::string directory = fprConfigDirectory();
  FprConfigDirectoryGuard guard(directory);
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", directory };
  ScopedEnvironmentVariable bootstrapName{ "BOOTSTRAP_CONFIG_NAME", fprBootstrapName };
  fprWriteBootstrap(directory, R"(<Configuration>
    <Properties>
        <Property name="fprInt" type="int32_t" value="42" process="fprProcess" class="FprService" instance="__DEFAULT"/>
    </Properties>
</Configuration>)");

  auto envConfig = std::make_shared<EnvironmentConfiguration>();
  auto configuration = fprConfiguration(envConfig);
  FilePropertyRepository repository(configuration, std::make_shared<ProcessName>("fprProcess"));

  REQUIRE_FALSE(repository.isEnabled());
  REQUIRE_FALSE(repository.isMutable());
  REQUIRE_FALSE(repository.isShadow());
  // the configured defaults are still delivered
  REQUIRE(repository.awake().size() == 1);
}
