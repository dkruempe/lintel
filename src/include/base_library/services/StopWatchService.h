#ifndef CPP_BASIC_LIBRARIES_STOPWATCHSERVICE_H
#define CPP_BASIC_LIBRARIES_STOPWATCHSERVICE_H

#include <chrono>
#include <ostream>

class StopWatchService {
public:
  /**
   * @brief constructor for timer. shutdown implies the start or not start of
   * the timer
   *
   * @param run   variable for choose to start or not start the timer
   * @author      Example User
   * @date        2019-01-30
   */
  explicit StopWatchService(bool run = true);

  /**
   * @brief reset timer if started or start timer if it was not started
   * @author      Example User
   * @date        2019-01-30
   */
  void reset();

  /**
   * @brief possibility to stop timer
   * @author      Example User
   * @date        2019-01-30
   */
  void stop();

  /**
   * @brief return elapsed ms of timer
   *
   * @return      Example User
   * @date        2019-01-30
   */
  [[nodiscard]] std::chrono::nanoseconds elapsed() const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const StopWatchService &service);

private:
  // start point of timer
  std::chrono::time_point<std::chrono::steady_clock> startTime;
  std::chrono::time_point<std::chrono::steady_clock> endTime;
  bool stopVar = false;
  bool run;
};

#endif // CPP_BASIC_LIBRARIES_STOPWATCHSERVICE_H
