# TODOs

1. Create wrapper for filesystem operations
2. Create MultiFileLogger which automatic manages logger depending on input loggerName
3. Create KernelLog appender to reduce log input

## 1. FileSystemOperations 

```objectivec
/**
 * Class for operations with file
 */
class File {
    /**
     * Create new file. Checks if the given path exists if not creates the directories also
     */
    static void create(std::filesystem::path file);
    
    /**
     * Checks if the given path exists and if the path is a file 
     */
    static bool exists (std::filesystem::path file);
    
    /**
     * Reads the complete file
     */
    static std::string read (std::filesystem::path file);

    /**
     * writes the content to the given file (only if file not exists)
     */
    static void write (std::filesystem::path file, std::string content);

    /**
     * append/write creates new file if not exists and appends the next content
     */
    static void append (std::filesytem::path file, std::string content);

    /**
     * removes the given file
     */
    static boolean remove (std::filesystem::path file);

    /**
     * reads the next given line
     */
    static int readNextLine (std::filesytem::path file, int line);
};

class Directory {
    /** 
     * creates directory recursive
     */
    static void create(boolean recursive, std::filesystem::path directory);
    
    /**
     * checks if the directory exists and is directory
     */
    static boolean exists (std::filesystem::path directory);
    
    /**
     * removes directory recursive (if enabled)
     */
    static void remove(boolean recursive, std::filesytem::path directory);
};
```

## 2. MultiFileLogger

Features of MultiFileLogger:
* Logger creates automatic needed log4cxx logger
* uses template configuration file as basic configuration
* supports interface for specific manual changes / interactions (change only for one logger configuration etc.)
* automatic creates logger

## 3. KernelLog features

* use special log level for rescheduled messages to support a special functionality of filtering multiple entries in a
  defined timeframe

Example:
1. Hello World
2. Hello World 
3. Hello World

=> Hello World (3 times) in (x minutes) will produce only one log entry to reduce log spam