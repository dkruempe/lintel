#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/property/configuration/PropertyRepositoryComponent.h"
#include "lintel/features/property/configuration/PropertyRepositoryEntry.h"
#include "lintel/features/property/services/PropertyService.h"

namespace lintel {
namespace {

  constexpr const char *kName = "benchmark_property";
  constexpr const char *kInstanceName = "benchmark_instance";
  constexpr const char *kClassName = "BenchmarkClass";
  constexpr const char *kProcessName = "benchmark_process";

  /**
   * Minimal enabled+mutable repository. Without one, PropertyService logs an error
   * on every write, which would dominate the measured set hotpath.
   */
  class BenchPropertyRepository : public PropertyRepository
  {
  public:
    using PropertyRepository::PropertyRepository;

    DataStorage getDataStorage() override { return DataStorage(getType(), "benchmark"); }

    void save(const std::vector<std::shared_ptr<PropertyBase>> &) override {}

    void save(std::shared_ptr<PropertyBase>) override {}

    std::vector<std::shared_ptr<PropertyBase>> awake() override { return {}; }
  };

  std::shared_ptr<PropertyService> makeService()
  {
    // Point Configuration at a directory without a bootstrap.xml, otherwise it
    // would parse the real cfg/ (and fail: the benchmark has no components).
    ::setenv("CONFIG_DIRECTORY", "/tmp/lintel_benchmark_nonexistent_cfg", 1);
    auto environment = std::make_shared<EnvironmentConfiguration>();
    auto configuration = std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, environment);
    auto entry = std::make_shared<PropertyRepositoryEntry>(
      type_name<PropertyRepositoryComponent>(), PropertyRepositoryType::DEFAULT, true, false);
    configuration->setEntries({ entry });
    auto repository = std::make_shared<BenchPropertyRepository>(PropertyRepositoryType::DEFAULT, configuration);
    return std::make_shared<PropertyService>(std::vector<std::shared_ptr<PropertyRepository>>{ repository },
      std::vector<std::shared_ptr<AbstractServiceInterface>>{});
  }

}// namespace

static void BM_PropertyService_GetOrCreateExisting(benchmark::State &state)
{
  auto service = makeService();
  service->getOrCreate<int32_t>(kName, kInstanceName, kClassName, kProcessName, "", true, 0);
  for (auto _ : state) {
    auto property = service->getOrCreate<int32_t>(kName, kInstanceName, kClassName, kProcessName, "", true, 0);
    benchmark::DoNotOptimize(property);
  }
}
BENCHMARK(BM_PropertyService_GetOrCreateExisting);

static void BM_PropertyService_Get(benchmark::State &state)
{
  auto service = makeService();
  service->getOrCreate<int32_t>(kName, kInstanceName, kClassName, kProcessName, "", true, 0);
  for (auto _ : state) {
    auto &property = service->get(kName, kInstanceName, kClassName, kProcessName);
    benchmark::DoNotOptimize(property);
  }
}
BENCHMARK(BM_PropertyService_Get);

static void BM_PropertyService_ChangeValueOf(benchmark::State &state)
{
  auto service = makeService();
  auto property = service->getOrCreate<int32_t>(kName, kInstanceName, kClassName, kProcessName, "", true, 0);
  int32_t value = 0;
  for (auto _ : state) {
    service->changeValueOf<int32_t>(property, ++value);
    benchmark::DoNotOptimize(value);
  }
}
BENCHMARK(BM_PropertyService_ChangeValueOf);

static void BM_PropertyService_AllOf(benchmark::State &state)
{
  auto service = makeService();
  for (int i = 0; i < 32; ++i) {
    service->getOrCreate<int32_t>(kName + std::to_string(i), kInstanceName, kClassName, kProcessName, "", true, i);
  }
  for (auto _ : state) {
    auto properties = service->allOf();
    benchmark::DoNotOptimize(properties);
  }
}
BENCHMARK(BM_PropertyService_AllOf);
}// namespace lintel
