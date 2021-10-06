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

Note, if you need only the library, you'll just have to use the src directory directly. So, you don't have to execute
the compilation for other directories "tests, example".

## 2 Dependencies

1. Log4cxx library (Logging library)
2. Fmt library (Will be removed with c++20 if available)
3. Catch2 UnitTest Framework
4. tinyxml2 Library

## 3 Examples

This directory contains some example how the library can be used.

## 4 Tests

This directory contains given tests for the library.