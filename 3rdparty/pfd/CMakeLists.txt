cmake_minimum_required(VERSION 3.1.0)

project(portable_file_dialogs VERSION 1.00 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(example examples/example.cpp)
add_executable(kill examples/kill.cpp)