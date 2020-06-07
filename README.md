# cpp-basic-libraries

##1 Description

This library implements some basic libraries for a faster start in cpp project. The following basic functionalities are
currently provided:

1. Logger/MultiFileLogger

   Basic Implementation for fast integration in a working project. In case of 
   the need to log in multiple files a wrapper for this is also available.
    
2. File/Directory operations wrapper

    This class wrapper wraps the std::filesystem library and some basic io operations.
    Now a file can easier be created/removed/copied or symlinked.
    
3. SchedulerService

    This class implements a basic SchedulerService for schedule tasks to avoid endless loops 
    or polling.
    
##2 Dependencies

1. Log4cxx library (Logging library)
2. Fmt library (Will be removed with c++20 if available)