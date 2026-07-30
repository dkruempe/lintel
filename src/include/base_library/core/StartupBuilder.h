#ifndef CPP_BASE_LIBRARY_STARTUPBUILDER_H
#define CPP_BASE_LIBRARY_STARTUPBUILDER_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/Feature.h"
#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConfigurationComponentBuilder.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessName.h"

class StartupBuilder {
private:
    std::vector<std::shared_ptr<FeatureInterface>> m_featureVec;
    std::shared_ptr<Hypodermic::Container> m_container = nullptr;
    std::shared_ptr<ProcessName> m_name;
    std::vector<std::shared_ptr<AbstractServiceInterface>> m_abstractServices;
    std::thread m_signalThread;
    std::condition_variable m_conditionVariable;
    std::atomic_bool m_stop = false;
    std::vector<std::string> m_arguments;
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration =
            nullptr;
    std::shared_ptr<ConfigurationComponentBuilder>
            m_configurationComponentBuilder;
    std::shared_ptr<Configuration> m_configuration = nullptr;
    std::shared_ptr<Features> m_features = std::make_shared<Features>();
    bool m_bootStrapServiceActive = true;
    std::mutex mutex;

public:
    StartupBuilder(ProcessName &&processName,
                   std::vector<std::string> &&arguments);

    StartupBuilder() = delete;

    static std::shared_ptr<StartupBuilder> with(int argc, char *argv[]);

    void addConfigurationComponent(std::shared_ptr<Component> &&component);

    template<typename FEATURE>
    void addFeature() {
        m_featureVec.push_back(std::make_shared<FEATURE>(m_features));
    }

    void disableBootstrapService();

    void withOutFeature(std::string_view nameOfFeature);

    void overrides(EnvironmentConfiguration::Environment environment, std::string value);

    void onShutdown();

    void signalThreadLoop();

    void start();
};

#endif  // CPP_BASE_LIBRARY_STARTUPBUILDER_H
