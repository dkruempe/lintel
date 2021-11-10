# TODOs

- [x] general: replace log4cxx with spdlog. Note for this spdlog need to have key system like log4cxx::MDC
- [ ] general: automatic create conan package.
- [ ] general: find general std::size_t serialize solution instead of limiting it to __APPLE__
- [ ] http: implement partly better exception handling
- [ ] http: support of paging
- [ ] db: change std::string implementations to std::string_view for better performance
- [ ] db: switch partly to constexpr implementation ?!
- [x] db: support sqlite again with sql statements
- [ ] shm: cli provide interface
- [ ] process: provide cli interface
- [x] property: SharedMemoryRepository add read/write process lock
- [x] property: SharedMemroyRepository add update information about changes to all connected processes
- [x] property: cli adobt available commands to multi processes
- [x] property: add regex support to cli command like commands of UserM
- [ ] property: cleanup entries if default value is same no repository in shadow mode at startup
- [x] property: add configuration check of Propertyrepository configuration
- [x] property: only awake properties of current process