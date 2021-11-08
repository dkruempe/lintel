# TODOs

- [x] replace log4cxx with spdlog. Note for this spdlog need to have key system like log4cxx::MDC
- [ ] automatic create conan package. So, that this package can directly deployed into conan package server
- [ ] http: implement partly better exception handling
- [ ] db: change std::string implementations to std::string_view for better performance
- [ ] db: switch partly to constexpr implementation ?!
- [ ] http: support of paging
- [ ] cli: add shared memory and process service to cli
- [x] db: support sqlite again with sql statements
- [ ] property: SharedMemoryRepository add read/write process lock
- [ ] property: SharedMemroyRepository add update information about changes to all connected processes
- [ ] property: cli adobt available commands to multi processes
- [ ] property: add device name or host name to table of DatabasePropertyRepository
- [ ] property: add regex support to cli command like commands of UserM
- [ ] property: cleanup entries if default value is same no repository in shadow mode at startup
- [x] property: add configuration check of Propertyrepository configuration
- [x] property: only awake properties of current process
- [ ] shm: cli provide interface
- [ ] general: find general std::size_t serialize solution instead of limiting it to __APPLE__ 