cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

#dependencies
find_package(date REQUIRED)

# main example (full stack: HTTP + CLI + property + shared memory + workers)
add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_17)
target_include_directories(main PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(main PRIVATE kruempelmann::base_library project_warnings myproject_options)

# worker example (spawned by main through ProcessService, no database)
add_executable(worker worker.cpp)
target_compile_features(worker PUBLIC cxx_std_17)
target_include_directories(worker PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(worker PRIVATE kruempelmann::base_library project_warnings myproject_options)

# persistence example (PostgreSQL, standalone, no bootstrap config)
add_executable(db db.cpp)
target_compile_features(db PUBLIC cxx_std_17)
target_include_directories(db PRIVATE ${magic_enum_INCLUDE_DIRS} kruempelmann::base_library)
target_link_libraries(db PRIVATE date::date kruempelmann::base_library project_warnings myproject_options)

# property change example (shared memory repository + event bus, self-contained)
add_executable(property_change property_change.cpp)
target_compile_features(property_change PUBLIC cxx_std_17)
target_include_directories(property_change PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(property_change PRIVATE kruempelmann::base_library project_warnings myproject_options)

# event bus example (boost::interprocess bus, self-contained)
add_executable(event_bus event_bus.cpp)
target_compile_features(event_bus PUBLIC cxx_std_17)
target_include_directories(event_bus PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(event_bus PRIVATE kruempelmann::base_library project_warnings myproject_options)

# message queue example (boost::interprocess message_queue)
add_executable(message_queue message_queue.cpp)
target_compile_features(message_queue PUBLIC cxx_std_17)
target_include_directories(message_queue PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(message_queue PRIVATE kruempelmann::base_library project_warnings myproject_options)

# named pipe example (POSIX fifo, no library dependency)
add_executable(named_pipe_server named_pipe_server.cpp)
target_compile_features(named_pipe_server PUBLIC cxx_std_17)
target_link_libraries(named_pipe_server PRIVATE project_warnings myproject_options)

# cleanup example (removes leftover boost::interprocess named mutexes)
add_executable(cleanup cleanup.cpp)
target_compile_features(cleanup PUBLIC cxx_std_17)
target_include_directories(cleanup PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(cleanup PRIVATE kruempelmann::base_library project_warnings myproject_options)
