#include <base_library/core/StartupBuilder.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/property/PropertyFeature.h>

int main(int argc, char *argv[]) {
  StartupBuilder &builder = StartupBuilder::with(argc, argv)
                                .addFeature<PropertyFeature>()
                                .addFeature<BaseFeature>()
                                .start();
  return 0;
}