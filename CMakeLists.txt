cmake_minimum_required(VERSION 3.14)
project(TaskScheduler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC (Visual Studio 2019) needs explicit /std:c++20 flag
if(MSVC)
  add_compile_options(/std:c++20 /Zc:__cplusplus)
endif()

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.16.1 
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backend/include)

add_executable(task_schedular
    backend/src/main.cpp
    backend/src/thread_pool.cpp
    backend/src/task_scheduler.cpp
)

target_link_libraries(task_schedular PRIVATE httplib::httplib nlohmann_json::nlohmann_json)