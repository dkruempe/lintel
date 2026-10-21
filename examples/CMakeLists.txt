cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# main example (full stack: HTTP + CLI + property + shared memory + workers)
add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_20)
target_include_directories(main PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(main PRIVATE lintel::lintel project_warnings lintel_options)

# worker example (spawned by main through ProcessService, no database)
add_executable(worker worker.cpp)
target_compile_features(worker PUBLIC cxx_std_20)
target_include_directories(worker PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(worker PRIVATE lintel::lintel project_warnings lintel_options)

# persistence example (PostgreSQL, standalone, no bootstrap config)
add_executable(db db.cpp)
target_compile_features(db PUBLIC cxx_std_20)
target_include_directories(db PRIVATE ${magic_enum_INCLUDE_DIRS} lintel::lintel)
target_link_libraries(db PRIVATE lintel::lintel project_warnings lintel_options)

# property change example (shared memory repository + event bus, self-contained).
# Forks a child process to prove cross-process propagation, so POSIX-only.
if(UNIX)
  add_executable(property_change property_change.cpp)
  target_compile_features(property_change PUBLIC cxx_std_20)
  target_include_directories(property_change PRIVATE ${magic_enum_INCLUDE_DIRS})
  target_link_libraries(property_change PRIVATE lintel::lintel project_warnings lintel_options)
endif()

# event bus example (boost::interprocess bus, self-contained).
# Forks a child process, so POSIX-only.
if(UNIX)
  add_executable(event_bus event_bus.cpp)
  target_compile_features(event_bus PUBLIC cxx_std_20)
  target_include_directories(event_bus PRIVATE ${magic_enum_INCLUDE_DIRS})
  target_link_libraries(event_bus PRIVATE lintel::lintel project_warnings lintel_options)
endif()

# message queue example (boost::interprocess message_queue)
add_executable(message_queue message_queue.cpp)
target_compile_features(message_queue PUBLIC cxx_std_20)
target_include_directories(message_queue PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(message_queue PRIVATE lintel::lintel project_warnings lintel_options)

# message queue request/reply example (typed call()/serve() over MessageQueue)
add_executable(message_queue_rpc message_queue_rpc.cpp)
target_compile_features(message_queue_rpc PUBLIC cxx_std_20)
target_include_directories(message_queue_rpc PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(message_queue_rpc PRIVATE lintel::lintel project_warnings lintel_options)

# named pipe example (POSIX fifo via mkfifo(), no library dependency)
if(UNIX)
  add_executable(named_pipe_server named_pipe_server.cpp)
  target_compile_features(named_pipe_server PUBLIC cxx_std_20)
  target_link_libraries(named_pipe_server PRIVATE project_warnings lintel_options)
endif()

# cleanup example (removes leftover boost::interprocess named mutexes)
add_executable(cleanup cleanup.cpp)
target_compile_features(cleanup PUBLIC cxx_std_20)
target_include_directories(cleanup PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(cleanup PRIVATE lintel::lintel project_warnings lintel_options)
