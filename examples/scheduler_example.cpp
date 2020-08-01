#include <base_library/Logger.h>
#include <base_library/services/SchedulerService.h>
#include <libgen.h>

int main(int argc, char *argv[]) {
  DECLARE_LOGGER(basename(argv[0]));
  SchedulerService scheduler(10);

  LOG_DEBUG("before scheduler start");
  std::function<void()> func = [&]() { LOG_INFO("Hello World"); };

  std::function<void()> schedule_after = [&]() { LOG_DEBUG("schedule after"); };

  std::function<void()> schedule = [&]() { LOG_DEBUG("direct schedule"); };

  std::function<void()> scheduleAt = [&]() { LOG_DEBUG("schedule_at"); };
  scheduler.schedule_at(
      std::chrono::steady_clock::now() + std::chrono::seconds(5), scheduleAt);
  scheduler.schedule(schedule);
  scheduler.schedule_after(std::chrono::seconds(10), schedule_after);
  scheduler.schedule_at_fixed_rate(std::chrono::seconds(1),
                                   std::chrono::seconds(1), func);

  std::this_thread::sleep_for(std::chrono::seconds(30));

  LOG_DEBUG("finished sleep");
  return 0;
}