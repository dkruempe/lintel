# TODOs

1. Property
    1. Implement PropertyRepository. After creation of Property load direct configuration from Repository.
       Repository loads after construction all configured Properties. Later the Repository only updates in 
       defined intervals via (polling, tcp connection information, file watcher, ...)
    2. Integration of Repository into PropertyService to set the correct values after creation
    3. Implement macro for defining Property to reduce coding overhead
    4. Simplify define of Property (marco ? )
2. Create MultiFileLogger which automatic manages logger depending on input loggerName
    1. Move Example Code to Class Implementation
    2. Implement default appender usage if Property for this logger is not available
3. Refactoring of Logger
    1. Logger::repeat use std::chrono to save from value (usage of new library for toString method?)