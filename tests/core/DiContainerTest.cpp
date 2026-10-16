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

  /** Concrete type with a default constructor - the container can auto-wire it. */
  class DiAutoWired
  {
  public:
    [[nodiscard]] int value() const { return 42; }
  };

  /** Concrete type asking for every registered GroupProvider as a vector. */
  class DiVectorProbe
  {
  private:
    std::vector<std::shared_ptr<GroupProvider>> m_providers;

  public:
    explicit DiVectorProbe(std::vector<std::shared_ptr<GroupProvider>> providers) : m_providers(std::move(providers)) {}

    [[nodiscard]] std::vector<std::shared_ptr<GroupProvider>> providers() const { return m_providers; }
  };

  /** Abstract type - not registered and not auto-wirable, so resolving it has to throw. */
  class DiUnregistered
  {
  public:
    virtual ~DiUnregistered() = default;
    virtual void pure() = 0;
  };

  class DiCircularB;

  class DiCircularA
  {
  public:
    explicit DiCircularA(std::shared_ptr<DiCircularB> b) : m_b(std::move(b)) {}

    std::shared_ptr<DiCircularB> m_b;
  };

  class DiCircularB
  {
  public:
    explicit DiCircularB(std::shared_ptr<DiCircularA> a) : m_a(std::move(a)) {}

    std::shared_ptr<DiCircularA> m_a;
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

TEST_CASE("DI container: auto-wires an unregistered concrete type", "[di_container]")
{
  lintel::di::ContainerBuilder builder;
  auto provider = std::make_shared<GroupProvider>();
  provider->add(Group("Auto-Wired", {}, true));
  builder.registerInstance(provider);
  std::shared_ptr<lintel::di::Container> container = builder.build();

  std::shared_ptr<DiAutoWired> autoWired = container->resolve<DiAutoWired>();
  REQUIRE(autoWired != nullptr);
  REQUIRE(autoWired->value() == 42);

  // the vector constructor parameter is filled through resolveAll
  std::shared_ptr<DiVectorProbe> probe = container->resolve<DiVectorProbe>();
  REQUIRE(probe != nullptr);
  REQUIRE(probe->providers().size() == 1);
  REQUIRE(probe->providers().front() == provider);
}

TEST_CASE("DI container: resolving an unregistered abstract type throws", "[di_container]")
{
  lintel::di::ContainerBuilder builder;
  std::shared_ptr<lintel::di::Container> container = builder.build();

  REQUIRE_THROWS_AS(container->resolve<DiUnregistered>(), lintel::di::ResolutionException);
}

TEST_CASE("DI container: a circular dependency is reported", "[di_container]")
{
  lintel::di::ContainerBuilder builder;
  builder.registerType<DiCircularA>();
  builder.registerType<DiCircularB>();
  std::shared_ptr<lintel::di::Container> container = builder.build();

  REQUIRE_THROWS_AS(container->resolve<DiCircularA>(), lintel::di::ResolutionException);
}

}// namespace lintel
