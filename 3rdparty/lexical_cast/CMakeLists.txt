cmake_minimum_required (VERSION 3.24 FATAL_ERROR)
project (lexical_cast VERSION 0.1 LANGUAGES C CXX ASM)

# Make including this file idempotent
#
if(TARGET lexical_cast OR TARGET lexical_cast::lexical_cast)
  return()
endif()

# If this is the current project.
#
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
  # Laod the cmake recipes and utilities.
  #
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/load_cmake_helpers.cmake)

  # Options for generating tests and documentation
  #
  option(LEXICAL_CAST_TEST "Generate the tests." ON)
  option(LEXICAL_CAST_DOCS "Generate the docs." OFF)

  # compile_commands.json
  #
  symlink_compile_commands()

  # Convenience targets for git operations
  #
  make_submodule_pull_target()

  # Setup compilation parameters
  #
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  
  add_compile_options(-Wall)
  add_compile_options(-g)
  add_compile_options(-O)

else()
  option(LEXICAL_CAST_TEST "Generate the tests." OFF)
  option(LEXICAL_CAST_DOCS "Generate the docs." OFF)
endif()

# Put executables in the top-level binary directory
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Output configuration
#
message("-- lexical_cast: Configuration: ${CMAKE_CURRENT_SOURCE_DIR}")
message("-- lexical_cast: Included from: ${CMAKE_SOURCE_DIR}")
message("-- lexical_cast: Install prefix: ${CMAKE_INSTALL_PREFIX}")
message("-- lexical_cast: test ${LEXICAL_CAST_TEST}")
message("-- lexical_cast: docs ${LEXICAL_CAST_DOCS}")

# Create the library as header only
#
add_library(lexical_cast INTERFACE)
add_library(lexical_cast::lexical_cast ALIAS lexical_cast)

# Define the header files for this library.
#
file(GLOB_RECURSE PUBLIC_INCLUDE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/*.h)
target_sources(lexical_cast INTERFACE FILE_SET HEADERS BASE_DIRS include FILES ${PUBLIC_INCLUDE_FILES})

target_include_directories(lexical_cast INTERFACE include)
target_link_libraries(lexical_cast INTERFACE fmt::fmt)

foreach(prog
    lexical_cast_builtin
    lexical_cast_dictionary
    lexical_cast_person
    lexical_cast_point
    lexical_cast_stdlib
    lexical_cast0
    lexical_cast1
    )
  add_executable(${prog} src/tools/${prog}.cpp)
  target_link_libraries(${prog} lexical_cast::lexical_cast)
endforeach()

# Optionally configure the tests
#
if(LEXICAL_CAST_TEST)
 add_gtest()
 add_subdirectory(test)
endif()

# Optionally configure the documentation
#
if(LEXICAL_CAST_DOCS)
  add_subdirectory(docs)
endif()

# Installation
#
#install_config(lexical_cast)
