# TODOs

- [ ] automatic create conan package. So, that this package can directly deployed into conan package server
- [x] add PropertyController example for rest connection. Goal is here to add an example for an REST Controller for the
  PropertyService. Note, there will be no basic implementation for a PropertyController. Maybe some extensions to create
  easier DTO objects that's it.
- [ ] replace log4cxx with spdlog. Note for this spdlog need to have key system like log4cxx::MDC
- [x] database implementation for postgres optimizations

      - change string to string_view for performance 
      - switch to constexpr ?
      - better exception handling