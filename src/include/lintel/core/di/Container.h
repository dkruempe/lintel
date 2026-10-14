#ifndef LINTEL_CORE_DI_CONTAINER_H
#define LINTEL_CORE_DI_CONTAINER_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lintel/core/utils/TypeName.h"

namespace lintel {
namespace di {

  class Container;
  class ContainerBuilder;

  /** Raised when a requested type has no registration and cannot be auto-wired. */
  class ResolutionException : public std::runtime_error
  {
  public:
    /** @param message human readable description of the failed resolution */
    explicit ResolutionException(const std::string &message) : std::runtime_error(message) {}
  };

  namespace detail {

    /** Maximum number of constructor parameters that can be injected. */
    constexpr std::size_t kMaxConstructorArguments = 20;

    /** Types a constructor parameter may ask the container for. */
    template<typename T> struct IsSupportedArgument : std::false_type
    {
    };

    template<typename T> struct IsSupportedArgument<std::shared_ptr<T>> : std::true_type
    {
    };

    template<typename T> struct IsSupportedArgument<std::vector<std::shared_ptr<T>>> : std::true_type
    {
    };

    /** Resolves the single dependency that a supported constructor parameter denotes. */
    template<typename TDependency> struct ArgumentResolver;

    template<typename TDependency> struct ArgumentResolver<std::shared_ptr<TDependency>>
    {
      template<typename TContainer> static std::shared_ptr<TDependency> resolve(TContainer &container)
      {
        return container.template resolve<TDependency>();
      }
    };

    template<typename TDependency> struct ArgumentResolver<std::vector<std::shared_ptr<TDependency>>>
    {
      template<typename TContainer> static std::vector<std::shared_ptr<TDependency>> resolve(TContainer &container)
      {
        return container.template resolveAll<TDependency>();
      }
    };

    /** Universal constructor argument: every parameter asks the container for its own type. */
    template<typename TOwner> struct InjectorArgument
    {
      Container &container;

      template<typename T,
        typename =
          std::enable_if_t<IsSupportedArgument<std::decay_t<T>>::value && !std::is_convertible<TOwner, T>::value>>
      operator T() const
      {
        return ArgumentResolver<std::decay_t<T>>::resolve(container);
      }
    };

    /** The injector is repeated once per parameter; the index only distinguishes the pack. */
    template<typename T, std::size_t> using InjectorFor = InjectorArgument<T>;

    template<typename T, std::size_t... Indices> constexpr bool isInjectableWith(std::index_sequence<Indices...>)
    {
      return std::is_constructible_v<T, InjectorFor<T, Indices>...>;
    }

    template<typename T, std::size_t N> constexpr bool isInjectableWith()
    {
      return isInjectableWith<T>(std::make_index_sequence<N>{});
    }

    template<typename T, std::size_t N> constexpr std::size_t constructorArity()
    {
      if constexpr (N == 0) {
        if constexpr (std::is_constructible_v<T>) {
          return 0;
        } else {
          return constructorArity<T, 1>();
        }
      } else if constexpr (N > kMaxConstructorArguments) {
        return kMaxConstructorArguments + 1;
      } else if constexpr (isInjectableWith<T, N>()) {
        return N;
      } else {
        return constructorArity<T, N + 1>();
      }
    }

    /** True if T can be created by the container: default ctor or injectable parameters. */
    template<typename T> constexpr bool hasInjectableConstructor()
    {
      if constexpr (!std::is_class_v<T> || std::is_abstract_v<T>) {
        return false;
      } else {
        return constructorArity<T, 0>() <= kMaxConstructorArguments;
      }
    }

    template<typename T, std::size_t... Indices>
    std::shared_ptr<T> createWithInjectors(Container &container, std::index_sequence<Indices...>)
    {
      return std::make_shared<T>(InjectorFor<T, Indices>{ container }...);
    }

    template<typename T> std::shared_ptr<T> createInstance(Container &container)
    {
      return createWithInjectors<T>(container, std::make_index_sequence<constructorArity<T, 0>()>{});
    }

    /** Registrations currently being activated on this thread - used to break cycles. */
    inline std::vector<const void *> &activeRegistrations()
    {
      thread_local std::vector<const void *> active;
      return active;
    }

    /** Pushes the registration for the duration of its activation. */
    struct ActivationScope
    {
      explicit ActivationScope(const void *registration) : m_registration(registration)
      {
        auto &active = activeRegistrations();
        if (std::find(active.begin(), active.end(), m_registration) != active.end()) {
          throw ResolutionException("circular dependency detected while creating an instance");
        }
        active.push_back(m_registration);
      }
      ~ActivationScope() { activeRegistrations().pop_back(); }
      ActivationScope(const ActivationScope &) = delete;
      ActivationScope &operator=(const ActivationScope &) = delete;

    private:
      const void *m_registration;
    };

  }// namespace detail

  /** A single registration, shared by all keys it is exposed under. */
  struct Registration
  {
    /** Every key this registration is reachable through (self plus interfaces). */
    std::vector<std::type_index> keys;
    /** Whether the activated instance is cached and reused. */
    bool singleton = false;
    /** Creates the concrete instance; called once per activation (once in total for singletons). */
    std::function<std::shared_ptr<void>(Container &)> factory;
    /** Per key conversion of the concrete instance to the key's (possibly non-first) base pointer. */
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>(const std::shared_ptr<void> &)>> casts;
    /** Cached singleton instance, empty until the first activation. */
    std::shared_ptr<void> instance;
    /** Serializes the activation of a singleton. */
    std::mutex mutex;

    /** Convert the concrete instance pointer to the subobject addressed by @p key. */
    std::shared_ptr<void> castTo(const std::type_index &key, const std::shared_ptr<void> &concrete) const
    {
      auto found = casts.find(key);
      if (found == casts.end()) { return concrete; }
      return found->second(concrete);
    }
  };

  /** Resolves registered instances and auto-wires unregistered concrete types. */
  class Container
  {
  public:
    Container() = default;
    Container(const Container &) = delete;
    Container &operator=(const Container &) = delete;

    /** Resolve the last registration for T.
     * @tparam T the (interface or concrete) type to resolve
     * @return the activated instance
     * @throws ResolutionException if T is not registered and cannot be auto-wired */
    template<typename T> std::shared_ptr<T> resolve()
    {
      std::shared_ptr<Registration> registration;
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto found = m_registrations.find(std::type_index(typeid(T)));
        if (found != m_registrations.end() && !found->second.empty()) { registration = found->second.back(); }
      }
      if (!registration) { registration = tryAutoRegister<T>(); }
      if (!registration) {
        throw ResolutionException(std::string("no registration for ") + std::string(type_name<T>()));
      }
      return std::static_pointer_cast<T>(registration->castTo(std::type_index(typeid(T)), instantiate(registration)));
    }

    /** Resolve every registration for T in registration order.
     * @tparam T the interface type to resolve all implementations of
     * @return the activated instances, empty if T was never registered */
    template<typename T> std::vector<std::shared_ptr<T>> resolveAll()
    {
      std::vector<std::shared_ptr<Registration>> registrations;
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto found = m_registrations.find(std::type_index(typeid(T)));
        if (found != m_registrations.end()) { registrations = found->second; }
      }
      std::vector<std::shared_ptr<T>> instances;
      instances.reserve(registrations.size());
      for (const auto &registration : registrations) {
        instances.push_back(
          std::static_pointer_cast<T>(registration->castTo(std::type_index(typeid(T)), instantiate(registration))));
      }
      return instances;
    }

  private:
    friend class ContainerBuilder;

    void addRegistration(const std::shared_ptr<Registration> &registration);
    std::shared_ptr<void> instantiate(const std::shared_ptr<Registration> &registration);
    template<typename T> std::shared_ptr<Registration> tryAutoRegister();

    std::unordered_map<std::type_index, std::vector<std::shared_ptr<Registration>>> m_registrations;
    std::mutex m_mutex;
  };

  /** Fluent handle returned by ContainerBuilder::registerType<T>(). */
  template<typename TInstance> class RegistrationBuilder
  {
  public:
    /** @param registration the registration being configured */
    explicit RegistrationBuilder(std::shared_ptr<Registration> registration) : m_registration(std::move(registration))
    {}

    /** Also expose the instance under the given interface. */
    template<typename TInterface> RegistrationBuilder &as()
    {
      m_registration->keys.push_back(std::type_index(typeid(TInterface)));
      m_registration->casts[std::type_index(typeid(TInterface))] =
        [](const std::shared_ptr<void> &instance) -> std::shared_ptr<void> {
        return std::static_pointer_cast<void>(
          std::static_pointer_cast<TInterface>(std::static_pointer_cast<TInstance>(instance)));
      };
      return *this;
    }

    /** Explicit no-op: a registration is always reachable as its concrete type. */
    RegistrationBuilder &asSelf() { return *this; }

    /** Cache and reuse a single instance for all keys of this registration. */
    RegistrationBuilder &singleInstance()
    {
      m_registration->singleton = true;
      return *this;
    }

  private:
    std::shared_ptr<Registration> m_registration;
  };

  /** Collects registrations and builds an immutable Container from them. */
  class ContainerBuilder
  {
  public:
    /** Register a ready-made instance under its static type.
     * @tparam TInstance the concrete type of the instance */
    template<typename TInstance> void registerInstance(std::shared_ptr<TInstance> instance)
    {
      std::shared_ptr<void> shared = std::move(instance);
      auto registration = std::make_shared<Registration>();
      registration->keys.push_back(std::type_index(typeid(TInstance)));
      registration->casts[std::type_index(typeid(TInstance))] =
        [](const std::shared_ptr<void> &instance) -> std::shared_ptr<void> { return instance; };
      registration->singleton = true;
      registration->instance = shared;
      registration->factory = [shared](Container &) -> std::shared_ptr<void> { return shared; };
      m_registrations.push_back(std::move(registration));
    }

    /** Register a type for on-demand construction.
     * @tparam TInstance the concrete type; must be default-constructible or injectable */
    template<typename TInstance> RegistrationBuilder<TInstance> registerType()
    {
      static_assert(detail::hasInjectableConstructor<TInstance>(),
        "registerType<T> requires T to be default-constructible or to have a constructor whose parameters are "
        "std::shared_ptr<...> or std::vector<std::shared_ptr<...>>");
      auto registration = std::make_shared<Registration>();
      registration->keys.push_back(std::type_index(typeid(TInstance)));
      registration->casts[std::type_index(typeid(TInstance))] =
        [](const std::shared_ptr<void> &instance) -> std::shared_ptr<void> { return instance; };
      registration->factory = [](Container &container) -> std::shared_ptr<void> {
        return std::static_pointer_cast<void>(detail::createInstance<TInstance>(container));
      };
      m_registrations.push_back(registration);
      return RegistrationBuilder<TInstance>(registration);
    }

    /** Build the immutable container from everything registered so far.
     * @return the ready to use container */
    std::shared_ptr<Container> build()
    {
      auto container = std::make_shared<Container>();
      for (const auto &registration : m_registrations) { container->addRegistration(registration); }
      // The container is resolvable as itself so injected services and bootstrap
      // plugins receive the real container (e.g. VirtualGroupBootstrapPlugin
      // resolves all registered GroupProviders). A weak_ptr keeps this from
      // creating a container <-> registration reference cycle.
      auto selfRegistration = std::make_shared<Registration>();
      selfRegistration->keys.push_back(std::type_index(typeid(Container)));
      selfRegistration->casts[std::type_index(typeid(Container))] =
        [](const std::shared_ptr<void> &instance) -> std::shared_ptr<void> { return instance; };
      std::weak_ptr<Container> weak = container;
      selfRegistration->factory = [weak](Container &) -> std::shared_ptr<void> {
        return std::static_pointer_cast<void>(weak.lock());
      };
      container->addRegistration(selfRegistration);
      return container;
    }

  private:
    std::vector<std::shared_ptr<Registration>> m_registrations;
  };

  inline void Container::addRegistration(const std::shared_ptr<Registration> &registration)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &key : registration->keys) { m_registrations[key].push_back(registration); }
  }

  inline std::shared_ptr<void> Container::instantiate(const std::shared_ptr<Registration> &registration)
  {
    detail::ActivationScope scope(registration.get());
    if (!registration->singleton) { return registration->factory(*this); }
    std::lock_guard<std::mutex> lock(registration->mutex);
    if (!registration->instance) { registration->instance = registration->factory(*this); }
    return registration->instance;
  }

  template<typename T> std::shared_ptr<Registration> Container::tryAutoRegister()
  {
    if constexpr (!detail::hasInjectableConstructor<T>()) {
      return nullptr;
    } else {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto &registrations = m_registrations[std::type_index(typeid(T))];
      if (!registrations.empty()) { return registrations.back(); }
      auto registration = std::make_shared<Registration>();
      registration->keys.push_back(std::type_index(typeid(T)));
      registration->casts[std::type_index(typeid(T))] =
        [](const std::shared_ptr<void> &instance) -> std::shared_ptr<void> { return instance; };
      registration->factory = [](Container &container) -> std::shared_ptr<void> {
        return std::static_pointer_cast<void>(detail::createInstance<T>(container));
      };
      registrations.push_back(registration);
      return registration;
    }
  }

}// namespace di
}// namespace lintel

#endif// LINTEL_CORE_DI_CONTAINER_H
