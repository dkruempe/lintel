#include "base_library/features/base/configuration/HistoryComponent.h"

#include <cstring>

#include <base_library/core/utils/TypeName.h>
#include <tinyxml2.h>

#include "base_library/features/base/configuration/ConfigurationException.h"
#include "base_library/features/base/configuration/HistoryServiceEntry.h"

const HistoryComponent::Shapes HistoryComponent::shape{};

HistoryComponent::HistoryComponent() : Component(shape.CONFIG_ROOT) {}

std::vector<std::shared_ptr<Entry>> HistoryComponent::parse(
        const std::string &content, const std::string &fileName,
        int32_t lineOffset) {
    std::vector<std::shared_ptr<Entry>> entries;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return entries;
    }

    for (tinyxml2::XMLElement *historyElement = rootNode->FirstChildElement();
         historyElement != nullptr;
         historyElement = historyElement->NextSiblingElement()) {
        if (std::strcmp(historyElement->Name(), shape.HISTORY_ROOT) != 0) {
            continue;
        }
        const char *processName =
                historyElement->Attribute(shape.PROCESS_NAME);
        const char *queueName = historyElement->Attribute(shape.QUEUE);
        const char *maxMessagesStr =
                historyElement->Attribute(shape.MAX_MESSAGES);

        const int32_t lineNumber =
                historyElement->GetLineNum() + lineOffset - 1;

        if (processName == nullptr) {
            throw ConfigurationException(this->getConfigRoot(),
                                         "please set process_name for history "
                                         "service",
                                         lineNumber);
        }
        const std::string queue =
                queueName != nullptr ? std::string(queueName) : "history";
        const int32_t maxMessages =
                maxMessagesStr != nullptr ? std::stoi(maxMessagesStr) : 1000;

        entries.push_back(std::make_shared<HistoryServiceEntry>(
                type_name<HistoryComponent>(), std::string(processName), queue,
                maxMessages));
    }
    return entries;
}
