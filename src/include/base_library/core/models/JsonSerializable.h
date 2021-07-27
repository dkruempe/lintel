#ifndef CPP_BASE_LIBRARY_JSONSERIALIZABLE_H
#define CPP_BASE_LIBRARY_JSONSERIALIZABLE_H

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>

/**
 * Class for serialization and deserialization of Json Types
 */
class JsonSerializable {
 public:
  JsonSerializable() = default;
  virtual ~JsonSerializable() = default;
  /**
   * deserialization of given value in json
   * @param obj json value
   * @return success or not ?
   */
  virtual bool deserialize(const rapidjson::Value& obj) { return false; }

  /**
   * serialization of object to json itself
   * @param writer for serialization of object
   */
  virtual void serialize(
      rapidjson::Writer<rapidjson::StringBuffer>* writer) const = 0;

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

  virtual void deserialize(const std::string& json) {
    rapidjson::Document document;
    document.Parse(json.c_str());
    deserialize(document);
  }
};

#endif  // CPP_BASE_LIBRARY_JSONSERIALIZABLE_H
