#ifndef CPP_SYSTEM_LIBRARY_PROCESS_H
#define CPP_SYSTEM_LIBRARY_PROCESS_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base_library/core/utils/UUID.h"

class Process {
 private:
  // UUID
  std::string m_id = UUID::generate();
  // process
  std::filesystem::path m_path;
  std::vector<std::string> m_args;
  // configuration
  bool m_autoRestart = false;
  int32_t m_restarts = 0;
  int m_maxAutoRestarts = -1;
  // events
  std::shared_ptr<std::function<void(const Process &)>> m_onStart = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onStop = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onRestart = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onTerminate = nullptr;
  std::shared_ptr<std::function<void(const Process &)>> m_onFinish = nullptr;

 public:
  explicit Process(std::filesystem::path path, std::vector<std::string> args);

  // setter
  void addOnStartEvent(
      std::shared_ptr<std::function<void(const Process &)>> onStart);
  void addOnStopEvent(
      std::shared_ptr<std::function<void(const Process &)>> onStop);
  void addOnRestartEvent(
      std::shared_ptr<std::function<void(const Process &)>> onRestart);
  void addOnTerminateEvent(
      std::shared_ptr<std::function<void(const Process &)>> onTerminate);
  void addOnFinishEvent(
      std::shared_ptr<std::function<void(const Process &)>> onFinish);
  void enableAutoStart(int maxAutoRestarts = -1);
  void disableAutoStart();
  void increaseRestarts();
  [[nodiscard]] int currentRestarts() const;

  // getter
  [[nodiscard]] const std::string &getId() const;
  [[nodiscard]] const std::filesystem::path &getPath() const;
  [[nodiscard]] bool isAutoRestart() const;
  [[nodiscard]] int getMaxAutoRestarts() const;
  [[nodiscard]] const std::vector<std::string> &getArgs() const;

  // events
  void onStart();
  void onStop();
  void onRestart();
  void onTerminate();
  void onFinish();
};
#endif  // CPP_SYSTEM_LIBRARY_PROCESS_H
