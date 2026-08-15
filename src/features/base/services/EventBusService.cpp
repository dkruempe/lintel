#include "base_library/features/base/services/EventBusService.h"

#include <stdexcept>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/configuration/EventBusComponent.h"

EventBusService::EventBusService(
        const std::shared_ptr<Configuration> &configuration,
        std::shared_ptr<ProcessName> processName,
        std::shared_ptr<SharedMemoryService> sharedMemoryService,
        std::shared_ptr<SharedMemorySegmentManager> segmentManager)
        : AbstractService<EventBusService>(processName->getProcessName()),
          m_processName(std::move(processName)),
          m_sharedMemoryService(std::move(sharedMemoryService)),
          m_segmentManager(std::move(segmentManager)),
          m_configurationMap(init(configuration)) {}

std::map<std::string, std::shared_ptr<EventBusEntry>> EventBusService::init(
        const std::shared_ptr<Configuration> &configuration) {
    auto confs = configuration->configurationOf<EventBusComponent>();
    std::map<std::string, std::shared_ptr<EventBusEntry>> map;
    for (const auto &iter : confs) {
        auto entry = std::static_pointer_cast<EventBusEntry>(iter);
        map.insert({entry->getName(), entry});
        LOG_INFO("found event bus {}", entry->getName());
    }
    return map;
}

EventBusConfig EventBusService::configOf(
        const std::shared_ptr<EventBusEntry> &entry) {
    EventBusConfig config;
    config.busCapacity = entry->getBusCapacity();
    config.subscriberCapacity = entry->getSubscriberCapacity();
    config.maxTopics = entry->getMaxTopics();
    config.maxSubscribers = entry->getMaxSubscribers();
    if (!config.isValid()) {
        throw std::runtime_error("invalid event bus configuration for '" +
                                 entry->getName() + "'");
    }
    return config;
}

EventBus &EventBusService::resolve(
        const std::shared_ptr<EventBusEntry> &entry) {
    const std::size_t generation = m_sharedMemoryService->getGeneration();
    const auto cached = m_generations.find(entry->getName());
    if (cached != m_generations.end() && cached->second == generation) {
        return *m_buses.at(entry->getName());
    }
    auto segment = m_segmentManager->of(entry->getSegment());
    if (segment == nullptr) {
        LOG_ERROR("segment '{}' not found for event bus '{}'",
                  entry->getSegment(), entry->getName());
        throw std::runtime_error("segment not found for event bus " +
                                 entry->getName());
    }
    const EventBusConfig config = configOf(entry);
    const std::size_t required = EventBus::requiredSize(config);
    if (segment->getSize() < required) {
        LOG_WARN("segment '{}' size {} may be too small for event bus '{}' "
                 "(estimated {})",
                 segment->getName(), segment->getSize(), entry->getName(),
                 required);
    }
    auto *segmentManager = m_sharedMemoryService->getSegmentManager(segment);
    if (segmentManager == nullptr) {
        LOG_ERROR("segment '{}' not mapped for event bus '{}'",
                  entry->getSegment(), entry->getName());
        throw std::runtime_error("segment not mapped for event bus " +
                                 entry->getName());
    }
    EventBus &bus = m_sharedMemoryService->constructObjectWith<EventBus>(
            segment, entry->getName(), entry->getName(), config,
            segmentManager);
    m_buses[entry->getName()] = &bus;
    m_generations[entry->getName()] = generation;
    return bus;
}

std::shared_ptr<IEventBus> EventBusService::of(const std::string &name) {
    auto found = m_configurationMap.find(name);
    if (found == m_configurationMap.end()) {
        LOG_ERROR("no event bus found for {}", name);
        throw std::runtime_error("no event bus found for " + name);
    }
    EventBus &bus = resolve(found->second);
    return std::make_shared<EventBusView>(bus);
}

void EventBusService::onInitialize() {
    for (const auto &entry : m_configurationMap) {
        try {
            EventBus &bus = resolve(entry.second);
            const EventBusConfig config = bus.getConfig();
            LOG_INFO("event bus '{}' ready (bus capacity {}, subscriber "
                     "capacity {}, topics {}, subscribers {})",
                     bus.getName(), config.busCapacity,
                     config.subscriberCapacity, config.maxTopics,
                     config.maxSubscribers);
        } catch (const std::exception &exception) {
            LOG_ERROR("event bus '{}' initialization failed: {}",
                      entry.second->getName(), exception.what());
        }
    }
}
