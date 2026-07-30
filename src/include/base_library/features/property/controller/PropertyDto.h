#ifndef CPP_BASE_LIBRARY_PROPERTYDTO_H
#define CPP_BASE_LIBRARY_PROPERTYDTO_H

#include <memory>

#include "base_library/core/models/JsonSerializable.h"
#include "base_library/features/property/models/PropertyBase.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

/** DTO representing a single property, JSON-serializable */
class PropertyDto : public JsonSerializable {
private:
    std::string m_name;
    std::string m_instanceName;
    std::string m_className;
    std::string m_processName;
    bool m_runtimeChange = false;
    std::string m_description;
    PropertyRepositoryType m_repositoryType = PropertyRepositoryType::UNDEFINED;
    std::string m_extraInformation;
    std::string m_value;
    std::string m_type;

    static struct Shapes {
        const std::string NAME = "name";
        const std::string INSTANCE = "instance";
        const std::string CLASS = "class";
        const std::string PROCESS = "process";
        const std::string RUNTIME_CHANGE = "runtime_change";
        const std::string DESCRIPTION = "description";
        const std::string TYPE = "type";
        const std::string REPOSITORY_TYPE = "repository_type";
        const std::string VALUE = "value";
        const std::string EXTRA_INFORMATION = "extra_information";
    } m_shape;

public:
    /** @param property the property base to convert */
    explicit PropertyDto(const std::shared_ptr<PropertyBase> &property);

    PropertyDto() = default;

    /** @return property name */
    [[nodiscard]] const std::string &getName() const;

    /** @return instance name */
    [[nodiscard]] const std::string &getInstanceName() const;

    /** @return class name */
    [[nodiscard]] const std::string &getClassName() const;

    /** @return process name */
    [[nodiscard]] const std::string &getProcessName() const;

    /** @return true if runtime changes are supported */
    [[nodiscard]] bool isRuntimeChange() const;

    /** @return property description */
    [[nodiscard]] const std::string &getDescription() const;

    /** @return repository type */
    [[nodiscard]] const PropertyRepositoryType &getRepositoryType() const;

    /** @return extra information string */
    [[nodiscard]] const std::string &getExtraInformation() const;

    /** @return current value as string */
    [[nodiscard]] const std::string &getValue() const;

    /** @return type string */
    [[nodiscard]] const std::string &getType() const;

    void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

    bool deserialize(const rapidjson::Value &obj) override;
};

#endif  // CPP_BASE_LIBRARY_PROPERTYDTO_H
