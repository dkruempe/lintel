#ifndef LINTEL_EVENTSCHEMA_H
#define LINTEL_EVENTSCHEMA_H

#include <cstdint>
#include <string>

namespace lintel {

/** Schema descriptor for typed events. */
struct EventSchema
{
  std::uint16_t id = 0;
  std::uint16_t version = 1;
  std::string name;
  std::string description;
};

/** Simple schema registry for typed events. */
class EventSchemaRegistry
{
public:
  static EventSchemaRegistry &instance()
  {
    static EventSchemaRegistry registry;
    return registry;
  }

  void registerSchema(const EventSchema &schema) { m_schemas[schema.id] = schema; }
  const EventSchema *find(std::uint16_t id) const
  {
    auto it = m_schemas.find(id);
    if (it == m_schemas.end()) { return nullptr; }
    return &it->second;
  }

private:
  std::map<std::uint16_t, EventSchema> m_schemas;
};

}// namespace lintel
#endif// LINTEL_EVENTSCHEMA_H
