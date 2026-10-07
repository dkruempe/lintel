#ifndef LINTEL_JSONSERIALIZABLE_H
#define LINTEL_JSONSERIALIZABLE_H

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>

#include "lintel/features/http/service/HttpBadRequestException.h"

/**
 * Class for serialization and deserialization of Json Types
 */
class JsonSerializable {
public:
    /** Default constructor */
    JsonSerializable() = default;

    /** Virtual destructor */
    virtual ~JsonSerializable() = default;

    /**
     * deserialization of given value in json
     * @param obj json value
     * @return success or not ?
     */
    virtual bool deserialize(const rapidjson::Value &obj) { return false; }

    /**
     * serialization of object to json itself
     * @param writer for serialization of object
     */
    virtual void serialize(
            rapidjson::Writer<rapidjson::StringBuffer> *writer) const = 0;

    /**
     * helper function to serialize json directly to string without the knowledge
     * of rapidjson itself.
     * @return serialized string of object itself.
     */
    [[nodiscard]] virtual std::string serialize() const {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::Writer writer(stringBuffer);
        serialize(&writer);
        return std::string(stringBuffer.GetString());
    }

    /**
     * Deserializes the object from a JSON string.
     * @param json JSON string to deserialize
     */
    virtual void deserialize(const std::string &json) {
        rapidjson::Document document;
        document.Parse(json.c_str());
        if (document.HasParseError() || !(document.IsObject() || document.IsArray())) {
            throw HttpBadRequestException();
        }
        deserialize(document);
    }
};

#endif  // LINTEL_JSONSERIALIZABLE_H
