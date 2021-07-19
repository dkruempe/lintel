#include <base_library/core/StartupBuilder.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/cli/CommandLineFeature.h>
#include <base_library/features/http/HttpFeature.h>
#include <base_library/features/property/PropertyFeature.h>

int main(int argc, char *argv[]) {
  StartupBuilder &builder = StartupBuilder::with(argc, argv)
                                .addFeature<PropertyFeature>()
                                .addFeature<BaseFeature>()
                                .addFeature<CommandLineFeature>()
                                .addFeature<HttpFeature>()
                                .start();
  return 0;
}