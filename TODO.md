# TODOs
- [ ] automatic create conan package. So, that this package can directly deployed into conan package server
- [ ] add PropertyController example for rest connection. Goal is here to add an example for an REST Controller
      for the PropertyService. Note, there will be no basic implementation for a PropertyController. Maybe
      some extensions to create easier DTO objects that's it.
- [ ] replace log4cxx with spdlog. Note for this spdlog need to have key system like log4cxx::MDC
- [ ] database implementation for postgres optimizations
      
      - Result implement iterator
      - implement template for setting parameters and use StringifyService ?
      - implement class for parameters ?
      - implement general return struct for Result for supporting the iterator ?
      - change string to string_view for performance 
      - switch to constexpr ?
      - better exception handling
      - optimization for faster insert
            - support copy command
            - droping indices => insert => creating indices ?
            - ...