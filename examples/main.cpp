#include <base_library/core/StartupBuilder.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/cli/CommandLineFeature.h>
#include <base_library/features/http/HttpFeature.h>
#include <base_library/features/property/PropertyFeature.h>

int main(int argc, char *argv[]) {
  std::shared_ptr<StartupBuilder> builder = StartupBuilder::with(argc, argv);
  builder->addFeature<PropertyFeature>();
  builder->addFeature<BaseFeature>();
  builder->addFeature<CommandLineFeature>();
  builder->addFeature<HttpFeature>();
  builder->start();
  return 0;
}