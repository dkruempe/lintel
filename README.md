# cpp-base-library

## 1 Description

This library implements some basic libraries for a faster start in cpp project. The following basic functionalities are
currently provided:

1. Logger/MultiFileLogger

   Basic Implementation for fast integration in a working project. In case of the need to log in multiple files a
   wrapper for this is also available.

2. File/Directory operations wrapper

   This class wrapper wraps the std::filesystem library and some basic io operations. Now a file can easier be
   created/removed/copied or symlinked.

3. SchedulerService/ExecutorService

   This class implements a basic SchedulerService for schedule tasks to avoid endless loops or polling.

4. Property Implementation for easy usage of properties

    * includes a Repository with a FileRepository implementation and for testing an empty implementation
    * includes a PropertyService to initialize all properties
    * includes an interface AbstractService and marco to easy initialize the properties
    * includes a PropertyFactory for easy creating of properties. Needed for service and Repository
    * Note: Easy extensions for the Repository like for example a database implementation. The database implementation
      won't be part of this library to keep a high support to all operating systems
    * The serialization/deserialization of an Property with the current given PropertyRepository can be influenced with
      the ConfigSerializationStrategy.java. Currently XML is supported. Feel free to TOML, JSON or whatever.

5. StringifyService

   This service easy converts variables of given types to a string and backwards. This interface can be extended with
   the given marco.

6. TypeName util

   This util returns the real type of an object with given marcos like PRETTY_FUNCTION
7. CommandLineService
   
   Interactive command line service, which can be extended by easy components. For all basic functions is a component
   directly implemented. The service is http based, which implies a remote connection support.
   Of course a user management is also available, so that you can easily restrict the access to 
   special components.
8. Shared Memory Service
   Available basic implementation for shared memory usage itself. All basic objects for shared memory are available
   for usage itself.
9. BootstrapService
   To support a direct boot without any manual interaction is a bootstrap service implemented. This service
   also is available for the database initialization itself. 

Note, if you need only the library, you'll just have to use the src directory directly. So, you don't have to execute
the compilation for other directories "tests, example".

## 2 Dependencies

1. spdlog library 
2. Fmt library (Will be removed with c++20 if available)
3. Catch2 UnitTest Framework
4. tinyxml2 Library
5. boost interprocess library
6. magic enum
7. openssl
8. rapidjson 
9. tabulate
10. date
11. zlib

## 3 Examples

This directory contains some example how the library can be used.

## 4 Tests

This directory contains given tests for the library.