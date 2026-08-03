cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()

# playground
find_package(date REQUIRED)
add_executable(playground playground.cpp)
target_compile_features(playground PUBLIC cxx_std_23)
target_include_directories(playground PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(playground PRIVATE date::date kruempelmann::base_library)

add_executable(db db.cpp)
target_compile_features(db PUBLIC cxx_std_23)
target_include_directories(db PRIVATE ${magic_enum_INCLUDE_DIRS} kruempelmann::base_library)
target_link_libraries(db PRIVATE date::date kruempelmann::base_library)

# named_pipe_server example
add_executable(named_pipe_server named_pipe_server.cpp)
target_compile_features(named_pipe_server PUBLIC cxx_std_23)

# message queue
add_executable(message_queue message_queue.cpp)
target_compile_features(message_queue PUBLIC cxx_std_23)
target_include_directories(message_queue PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(message_queue PRIVATE kruempelmann::base_library)

# main example
add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_23)
target_include_directories(main PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(main PRIVATE kruempelmann::base_library)

# cleanup example
add_executable(cleanup cleanup.cpp)
target_compile_features(cleanup PUBLIC cxx_std_23)
target_include_directories(cleanup PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(cleanup PRIVATE kruempelmann::base_library)