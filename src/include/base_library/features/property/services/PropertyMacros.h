#ifndef CPP_BASE_LIBRARY_PROPERTYMACROS_H
#define CPP_BASE_LIBRARY_PROPERTYMACROS_H

/**
 * Plain (non-module) header containing the property convenience macros.
 * Macros cannot be exported from C++20 modules, so they live in a separate
 * header that both module interface units and module consumers can include.
 */
#define DEFINE_PROPERTY(name, type, defaultValue, description, runtime)     \
  std::shared_ptr<Property<type>> name =                                    \
      registerProperty<type>(std::string(#name), defaultValue, description, \
                             runtime, __FILE__, __LINE__)
#define LOAD_PROPERTIES()                       \
  if (propertyService != nullptr) {             \
    propertyService->getOrCreate(m_properties); \
  }

#endif// CPP_BASE_LIBRARY_PROPERTYMACROS_H
