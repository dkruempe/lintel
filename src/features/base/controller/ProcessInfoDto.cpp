#include "base_library/core/services/LoggerMacros.h"
#include "base_library/features/base/controller/ProcessInfoDto.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/models/ProcessInfo.h"

ProcessInfoDto::Shapes ProcessInfoDto::m_shape{};

ProcessInfoDto::ProcessInfoDto(const ProcessInfo &processInfo)
        : m_id(processInfo.getProcess()->getId()),
          m_path(processInfo.getProcess()->getPath()),
          m_args(processInfo.getProcess()->getArgs()),
          m_autoRestart(processInfo.getProcess()->isAutoRestart()),
          m_restarts(processInfo.getProcess()->currentRestarts()),
          m_maxAutoRestarts(processInfo.getProcess()->getMaxAutoRestarts()),
          m_processId(processInfo.getProcessId()),
          m_isRunning(processInfo.isRunning()),
          m_exitCode(processInfo.getExitCode()),
          m_groupName(processInfo.getGroupName()),
          m_groupId(processInfo.getGroupId()) {}

const std::string &ProcessInfoDto::getId() const { return m_id; }

const std::filesystem::path &ProcessInfoDto::getPath() const { return m_path; }

const std::vector<std::string> &ProcessInfoDto::getArgs() const {
    return m_args;
}

bool ProcessInfoDto::isAutoRestart() const { return m_autoRestart; }

int32_t ProcessInfoDto::getRestarts() const { return m_restarts; }

int32_t ProcessInfoDto::getMaxAutoRestarts() const { return m_maxAutoRestarts; }

pid_t ProcessInfoDto::getProcessId() const { return m_processId; }

bool ProcessInfoDto::isRunning() const { return m_isRunning; }

int32_t ProcessInfoDto::getExitCode() const { return m_exitCode; }

const std::string &ProcessInfoDto::getGroupName() const { return m_groupName; }

const std::string &ProcessInfoDto::getGroupId() const { return m_groupId; }

void ProcessInfoDto::serialize(
        rapidjson::Writer<rapidjson::StringBuffer> *writer) const {
    writer->StartObject();
    // ID
    writer->String(m_shape.ID);
    writer->String(m_id.c_str());
    // PATH
    writer->String(m_shape.PATH);
    writer->String(m_path.c_str());
    // ARGS
    writer->String(m_shape.ARGS);
    writer->StartArray();
    for (const auto &arg: m_args) {
        writer->StartObject();
        writer->String(m_shape.ARG);
        writer->String(arg.c_str());
        writer->EndObject();
    }
    writer->EndArray();
    // AUTO_RESTART
    writer->String(m_shape.AUTO_RESTART);
    writer->Bool(m_autoRestart);
    // RESTARTs
    writer->String(m_shape.RESTARTS);
    writer->Int(m_restarts);
    // MAX_RESTARTS
    writer->String(m_shape.MAX_RESTARTS);
    writer->Int(m_maxAutoRestarts);
    // PROCESS_ID
    writer->String(m_shape.PROCESS_ID);
    writer->Int(static_cast<int32_t>(m_processId));
    // RUNS
    writer->String(m_shape.RUNS);
    writer->Bool(m_isRunning);
    // EXIT_CODE
    writer->String(m_shape.EXIT_CODE);
    writer->Int(m_exitCode);
    // GROUP_NAME
    writer->String(m_shape.GROUP_NAME);
    writer->String(m_groupName.c_str());
    // GROUP_ID
    writer->String(m_shape.GROUP_ID);
    writer->String(m_groupId.c_str());
    writer->EndObject();
}

bool ProcessInfoDto::deserialize(const rapidjson::Value &obj) {
    bool success = true;
    // ID
    if (obj.HasMember(m_shape.ID)) {
        m_id = obj[m_shape.ID].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.ID);
    }
    // PATH
    if (obj.HasMember(m_shape.PATH)) {
        std::string pathStr = obj[m_shape.PATH].GetString();
        m_path = std::filesystem::path(pathStr);
    } else {
        success = false;
        LOG_ERROR("{} not defined in json seralization", m_shape.PATH);
    }
    // ARGS
    if (obj.HasMember(m_shape.ARGS)) {
        auto args = obj[m_shape.ARGS].GetArray();
        for (const auto &arg: args) {
            m_args.emplace_back(arg[m_shape.ARG].GetString());
        }
    } else {
        LOG_WARN("{} not defined in json serilization", m_shape.ARGS);
    }
    // AUTO_RESTART
    if (obj.HasMember(m_shape.AUTO_RESTART)) {
        m_autoRestart = obj[m_shape.AUTO_RESTART].GetBool();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization",
                  m_shape.AUTO_RESTART);
    }
    // RESTARTS
    if (obj.HasMember(m_shape.RESTARTS)) {
        m_restarts = obj[m_shape.RESTARTS].GetInt();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.RESTARTS);
    }
    // MAX_RESTARTS
    if (obj.HasMember(m_shape.MAX_RESTARTS)) {
        m_maxAutoRestarts = obj[m_shape.MAX_RESTARTS].GetInt();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization",
                  m_shape.MAX_RESTARTS);
    }
    // PROCESS_ID
    if (obj.HasMember(m_shape.PROCESS_ID)) {
        m_processId = obj[m_shape.PROCESS_ID].GetInt();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization",
                  m_shape.PROCESS_ID);
    }
    // RUNS
    if (obj.HasMember(m_shape.RUNS)) {
        m_isRunning = obj[m_shape.RUNS].GetBool();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.RUNS);
    }
    // EXIT_CODE
    if (obj.HasMember(m_shape.EXIT_CODE)) {
        m_exitCode = obj[m_shape.EXIT_CODE].GetInt();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization",
                  m_shape.EXIT_CODE);
    }
    // GROUP_NAME
    if (obj.HasMember(m_shape.GROUP_NAME)) {
        m_groupName = obj[m_shape.GROUP_NAME].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization",
                  m_shape.GROUP_NAME);
    }
    // GROUP_ID
    if (obj.HasMember(m_shape.GROUP_ID)) {
        m_groupId = obj[m_shape.GROUP_ID].GetString();
    } else {
        success = false;
        LOG_ERROR("{} not defined in json serialization", m_shape.GROUP_ID);
    }
    return success;
}
