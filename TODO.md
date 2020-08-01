# TODOs

- [ ] Property
    - [ ] Implement Property Repository
        - [x] Implement NoPropertyRepository to not save properties and just have it in memory
        - [ ] Implement DatabasePropertyRepository with trigger in case of database change
        - [x] Implement FilePropertyRepository
    - [x] Integration of PropertyRepository into PropertyService
    - [x] Implement Marcros and AbstractService for easy initialization
    - [ ] Implement Property Controller via REST for easy change of properties 
- [ ] Logger    
    - [ ] MultiFileLogger
    - [ ] Refactoring Logger
        - [ ] Logger::repeat use std::chrono to save from value (usage of new Library for toString method? )
    - [ ] configure configuration for logstash. So, that normal Logger and MultiFileLogger can use it