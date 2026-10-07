#ifndef LINTEL_PROPERTYAPI_H
#define LINTEL_PROPERTYAPI_H

#include <memory>
#include <optional>
#include <vector>

#include "lintel/features/http/provider/ClientProvider.h"
#include "lintel/features/property/controller/PropertyDto.h"

/** API client for property operations via HTTP */
class PropertyApi {
private:
    std::shared_ptr<Client> m_client;

public:
    explicit PropertyApi(const std::shared_ptr<ClientProvider> &clientProvider);

    /**
     * @param processName process name filter
     * @param className class name filter
     * @param instanceName instance name filter
     * @return list of matching properties
     */
    std::vector<PropertyDto> allOf(const std::string &processName,
                                   const std::string &className,
                                   const std::string &instanceName);

    /**
     * @return single property matching the given criteria
     */
    std::optional<PropertyDto> of(const std::string &processName,
                                  const std::string &className,
                                  const std::string &instanceName,
                                  const std::string &propertyName);

    /**
     * Update a property value
     * @param propertyDto the property to update
     * @param value the new value
     * @return true on success
     */
    bool updateOf(const PropertyDto &propertyDto, const std::string &value);
};

#endif  // LINTEL_PROPERTYAPI_H
