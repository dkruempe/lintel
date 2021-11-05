# TODOs

- [x] replace log4cxx with spdlog. Note for this spdlog need to have key system like log4cxx::MDC
- [ ] automatic create conan package. So, that this package can directly deployed into conan package server
- [ ] http: implement partly better exception handling
- [ ] db: change std::string implementations to std::string_view for better performance
- [ ] db: switch partly to constexpr implementation ?!
- [ ] http: support of paging
- [ ] cli: add shared memory and process service to cli
- [ ] db: support sqlite again with sql statements