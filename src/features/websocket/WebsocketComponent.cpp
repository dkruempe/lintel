#include "base_library/features/websocket/WebsocketComponent.h"

#include <tinyxml2.h>

#include <magic_enum.hpp>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/websocket/WebsocketEntry.h"
WebsocketComponent::Shapes WebsocketComponent::shape{};
WebsocketComponent::WebsocketComponent() : Component(shape.CONFIG_ROOT) {}
std::vector<std::shared_ptr<Entry>> WebsocketComponent::parse(
    const std::string& content, const std::string& fileName,
    int32_t lineOffset) {
  std::vector<std::shared_ptr<Entry>> entries;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());
  tinyxml2::XMLElement* rootNode =
      document.FirstChildElement(getConfigRoot().c_str());
  if (rootNode == nullptr) {
    return entries;
  }

  for (tinyxml2::XMLElement* element = rootNode->FirstChildElement();
       element != nullptr; element = element->NextSiblingElement()) {
    if (std::strcmp(element->Name(), shape.WEBSOCKET_ROOT.c_str()) != 0) {
      continue;
    }
    const char* address = element->Attribute(shape.ADDRESS.c_str());
    const char* portTemp = element->Attribute(shape.PORT.c_str());
    const char* typeName = element->Attribute(shape.TYPE.c_str());
    const char* name = element->Attribute(shape.NAME.c_str());

    int32_t lineNumber = element->GetLineNum() + lineOffset - 1;
    uint16_t port = std::stoul(portTemp);
    TYPE type = magic_enum::enum_cast<TYPE>(typeName).value_or(UNDEFINED);
    if (type == UNDEFINED) {
      LOG_ERROR("type {} not available at {}", typeName, lineNumber);
      continue;
    }

    entries.push_back(std::make_shared<WebsocketEntry>(
        type_name<WebsocketComponent>(), address, port, type == Server, name));
  }
  return entries;
}
