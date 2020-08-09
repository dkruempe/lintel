# TODOs

- [ ] Property
    - [x] Implement Property Repository
        - [x] Implement NoPropertyRepository to not save properties and just have it in memory
        - [x] Implement FilePropertyRepository
    - [x] Integration of PropertyRepository into PropertyService
    - [x] Implement Marcros and AbstractService for easy initialization
    - [ ] Implement Property Controller via REST for easy change of properties 
- [ ] Logger
    - [ ] Refactoring Logger
        - [ ] Logger::repeat use std::chrono to save from value (usage of new Library for toString method? )
    - [x] configure configuration for logstash. So, that normal Logger can use it