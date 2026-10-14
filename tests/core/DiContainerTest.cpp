#include <lintel/core/di/Container.h>

#include <catch2/catch_all.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "lintel/features/base/models/Group.h"
#include "lintel/features/base/provider/GroupProvider.h"

namespace lintel {
namespace {

  // `di` prefix: unity builds merge several test files into one translation unit
  // and share one anonymous namespace.
  /**
   * Receives the DI container by injection and collects all GroupProviders from
   * it - the same access pattern VirtualGroupBootstrapPlugin relies on. Guards
   * that a service gets the real container and not an empty auto-wired one.
   */
  class DiContainerProbe
  {
  private:
    std::shared_ptr<lintel::di::Container> m_container;

  public:
    explicit DiContainerProbe(std::shared_ptr<lintel::di::Container> container) : m_container(std::move(container)) {}

    std::shared_ptr<lintel::di::Container> container() { return m_container; }

    std::vector<std::shared_ptr<GroupProvider>> groupProviders() { return m_container->resolveAll<GroupProvider>(); }
  };

}// namespace

TEST_CASE("DI container: the built container resolves as itself", "[di_container]")
{
  lintel::di::ContainerBuilder builder;
  std::shared_ptr<lintel::di::Container> container = builder.build();

  REQUIRE(container->resolve<lintel::di::Container>() == container);
}

TEST_CASE("DI container: an injected container sees all registered GroupProviders", "[di_container]")
{
  lintel::di::ContainerBuilder builder;
  auto provider = std::make_shared<GroupProvider>();
  provider->add(Group("Admin-Property", {}, true));
  builder.registerInstance(provider);
  builder.registerType<DiContainerProbe>();
  std::shared_ptr<lintel::di::Container> container = builder.build();

  std::shared_ptr<DiContainerProbe> probe = container->resolve<DiContainerProbe>();

  REQUIRE(probe->container() == container);
  const std::vector<std::shared_ptr<GroupProvider>> providers = probe->groupProviders();
  REQUIRE(providers.size() == 1);
  REQUIRE(providers.front() == provider);
}

}// namespace lintel
