#include <libfswatch/c++/event.hpp>
#include <libfswatch/c++/monitor.hpp>
#include <libfswatch/c++/monitor_factory.hpp>
#include <libfswatch/c/cmonitor.h>
#include <thread>

void printEventFlags(const std::vector<fsw_event_flag> &eventFlags) {
  for (const fsw_event_flag &eventFlag : eventFlags) {
    std::string eventString;
    switch (eventFlag) {
    case NoOp:
      eventString = "No event has occured";
      break;
    case PlatformSpecific:
      eventString = "Platform-specific placeholder for event type that cannot "
                    "currently be mapped";
      break;
    case Created:
      eventString = "Created";
      break;
    case Updated:
      eventString = "Updated";
      break;
    case Removed:
      eventString = "Removed";
      break;
    case Renamed:
      eventString = "Renamed";
      break;
    case OwnerModified:
      eventString = "OwnerModified";
      break;
    case AttributeModified:
      eventString = "AttributeModified";
      break;
    case MovedFrom:
      eventString = "MovedFrom";
      break;
    case MovedTo:
      eventString = "MovedTo";
      break;
    case IsFile:
      eventString = "IsFile";
      break;
    case IsDir:
      eventString = "IsDir";
      break;
    case IsSymLink:
      eventString = "IsSymLink";
      break;
    case Link:
      eventString = "Link";
      break;
    case Overflow:
      eventString = "Overflow";
      break;
    }
    std::cout << eventString << std::endl;
  }
}

void eventCallback(const std::vector<fsw::event> &events, void *pointer) {
  for (const fsw::event &event : events) {
    printEventFlags(event.get_flags());
    std::cout << "path:" << event.get_path() << " time:" << event.get_time()
              << std::endl;
  }
}

int main(int argc, char *argv[]) {
  fsw_monitor_type type = fsw_monitor_type::system_default_monitor_type;
  std::vector<std::string> paths = {"/Users/dkruempe/cfg"};
  fsw::monitor *const active_monitor =
      fsw::monitor_factory::create_monitor(type, paths, eventCallback);
  //  std::vector<fsw_event_type_filter> event_filters;
  //  std::map<std::string, std::string> monitor_properties;
  //  std::vector<fsw::monitor_filter> filters;
  //  active_monitor->set_properties(monitor_properties);
  //  active_monitor->set_allow_overflow(false);
  //  active_monitor->set_latency(1.0);
  //  active_monitor->set_fire_idle_event(false);
  //  active_monitor->set_recursive(false);
  //  active_monitor->set_directory_only(false);
  //  active_monitor->set_event_type_filters(event_filters);
  //  active_monitor->set_filters(filters);
  //  active_monitor->set_follow_symlinks(false);
  //  active_monitor->set_watch_access(false);
  std::thread t([&]() { active_monitor->start(); });
  std::this_thread::sleep_for(std::chrono::seconds(30));
  active_monitor->stop();
  t.join();
  return 0;
}