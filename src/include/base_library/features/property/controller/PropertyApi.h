#ifndef CPP_BASE_LIBRARY_PROPERTYAPI_H
#define CPP_BASE_LIBRARY_PROPERTYAPI_H

#include <memory>
#include <optional>
#include <vector>

#include "base_library/features/http/provider/ClientProvider.h"
#include "base_library/features/property/controller/PropertyDto.h"

class PropertyApi {
 private:
  std::shared_ptr<Client> m_client;

 public:
  explicit PropertyApi(const std::shared_ptr<ClientProvider> &clientProvider);
  std::vector<PropertyDto> allOf();

  std::vector<PropertyDto> allOf(const std::string &processName);

  std::vector<PropertyDto> allOf(const std::string &processName,
                                 const std::string &className);

  std::vector<PropertyDto> allOf(const std::string &processName,
                                 const std::string &className,
                                 const std::string &instanceName);

  std::optional<PropertyDto> of(const std::string &processName,
                                const std::string &className,
                                const std::string &instanceName,
                                const std::string &propertyName);

  bool updateOf(const PropertyDto &propertyDto, const std::string &value);
};

#endif  // CPP_BASE_LIBRARY_PROPERTYAPI_H
