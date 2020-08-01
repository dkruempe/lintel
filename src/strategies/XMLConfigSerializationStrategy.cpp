#include "base_library/strategies/XMLConfigSerializationStrategy.h"
#include "base_library/factories/PropertyFactory.h"
#include <tinyxml2.h>
#include <vector>

#define CONFIG_ROOT "Config"
#define ELEMENT_NAME "name"
#define PROCESS_ROOT "Process"
#define CLASS_ROOT "Class"
#define INSTANCE_ROOT "Instance"
#define PROPERTY_ROOT "Property"
#define PROPERTY_TYPE "type"
#define PROPERTY_VALUE "value"

std::string XMLConfigSerializationStrategy::serialize(
    std::vector<std::shared_ptr<PropertyBase>> properties) {
  std::sort(properties.begin(), properties.end(),
            [](const std::shared_ptr<PropertyBase> &rhs,
               std::shared_ptr<PropertyBase> &lhs) { return *rhs < *lhs; });
  tinyxml2::XMLDocument document;
  tinyxml2::XMLPrinter printer;

  tinyxml2::XMLElement *element = document.NewElement(CONFIG_ROOT);
  tinyxml2::XMLElement *prevProcess = nullptr;
  tinyxml2::XMLElement *prevClass = nullptr;
  tinyxml2::XMLElement *prevInstance = nullptr;
  for (const auto &property : properties) {
    if (prevInstance == nullptr ||
        prevInstance->Attribute(ELEMENT_NAME) != property->getInstanceName()) {
      if (prevInstance != nullptr) {
        prevClass->InsertEndChild(prevInstance);
      }
      prevInstance = document.NewElement(INSTANCE_ROOT);
      prevInstance->SetAttribute(ELEMENT_NAME,
                                 property->getInstanceName().c_str());
    }
    if (prevClass == nullptr ||
        prevClass->Attribute(ELEMENT_NAME) != property->getClassName()) {
      if (prevClass != nullptr) {
        prevProcess->InsertEndChild(prevClass);
      }
      prevClass = document.NewElement(CLASS_ROOT);
      prevClass->SetAttribute(ELEMENT_NAME, property->getClassName().c_str());
    }

    if (prevProcess == nullptr ||
        prevProcess->Attribute(ELEMENT_NAME) != property->getProcessName()) {
      if (prevProcess != nullptr) {
        element->InsertEndChild(prevProcess);
      }
      prevProcess = document.NewElement(PROCESS_ROOT);
      prevProcess->SetAttribute(ELEMENT_NAME,
                                property->getProcessName().c_str());
    }
    tinyxml2::XMLElement *propertyElement = document.NewElement(PROPERTY_ROOT);
    propertyElement->SetAttribute(ELEMENT_NAME, property->getName().c_str());
    propertyElement->SetAttribute(PROPERTY_TYPE, property->getType().c_str());
    propertyElement->SetAttribute(PROPERTY_VALUE, property->toString().c_str());
    prevInstance->InsertEndChild(propertyElement);
  }

  prevClass->InsertEndChild(prevInstance);
  prevProcess->InsertEndChild(prevClass);
  element->InsertEndChild(prevProcess);
  document.InsertEndChild(element);

  document.Print(&printer);
  return std::string(printer.CStr());
}
std::vector<std::shared_ptr<PropertyBase>>
XMLConfigSerializationStrategy::deserialize(const std::string &content) {
  std::vector<std::shared_ptr<PropertyBase>> properties;
  tinyxml2::XMLDocument document;
  document.Parse(content.c_str());

  tinyxml2::XMLElement *rootNode = document.FirstChildElement();
  if (rootNode == nullptr) {
    return properties;
  }

  for (tinyxml2::XMLElement *processElement = rootNode->FirstChildElement();
       processElement != nullptr;
       processElement = processElement->NextSiblingElement()) {
    if (std::string(processElement->Name()) != PROCESS_ROOT) {
      continue;
    }
    std::string processName =
        std::string(processElement->Attribute(ELEMENT_NAME));
    for (tinyxml2::XMLElement *classElement = processElement->FirstChildElement();
         classElement != nullptr;
         classElement = classElement->NextSiblingElement()) {
      if (std::string(classElement->Name()) != CLASS_ROOT) {
        continue;
      }
      std::string className =
          std::string(classElement->Attribute(ELEMENT_NAME));
      for (tinyxml2::XMLElement *instanceElement =
          classElement->FirstChildElement();
           instanceElement != nullptr;
           instanceElement = instanceElement->NextSiblingElement()) {
        if (std::string(instanceElement->Name()) != INSTANCE_ROOT) {
          continue;
        }
        std::string instanceName =
            std::string(instanceElement->Attribute(ELEMENT_NAME));
        for (tinyxml2::XMLElement *propertyElement =
            instanceElement->FirstChildElement();
             propertyElement != nullptr;
             propertyElement = propertyElement->NextSiblingElement()) {
          if (std::string(propertyElement->Name()) != PROPERTY_ROOT) {
            continue;
          }
          std::string propertyName =
              std::string(propertyElement->Attribute(ELEMENT_NAME));
          std::string propertyType =
              std::string(propertyElement->Attribute(PROPERTY_TYPE));
          std::string propertyValue =
              std::string(propertyElement->Attribute(PROPERTY_VALUE));
          properties.push_back(PropertyFactory::Create(
              propertyName, instanceName, className, processName, propertyType,
              propertyValue));
        }
      }
    }
  }
  return properties;
}
