#ifndef CPP_BASE_LIBRARY_PROCESSINGREQUESTS_H
#define CPP_BASE_LIBRARY_PROCESSINGREQUESTS_H

#include <map>
#include <optional>
#include <string>

class ProcessingRequests {
 private:
  std::map<std::string, std::string> m_processingRequests;  // id, method

 public:
  ProcessingRequests() = default;

  /**
   * informs about new send requests with given method and id of requests
   * @param method of message
   * @param id of id
   */
  void waitingFor(const std::string &method, const std::string &id);

  /**
   * acknowledge the received message via the given id
   * @param id message id
   * @return method name of send requests
   */
  std::optional<std::string> acknowledgeOf(const std::string &id);
};

#endif  // CPP_BASE_LIBRARY_PROCESSINGREQUESTS_H
