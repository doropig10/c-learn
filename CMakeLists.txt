cmake_minimum_required(VERSION 3.16)
project(robot_monitor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(robot_monitor
    src/main.cpp
    src/robot.cpp
)

target_include_directories(robot_monitor PRIVATE include)

if(NOT MSVC)
    target_compile_options(robot_monitor PRIVATE -Wall -Wextra)
endif()