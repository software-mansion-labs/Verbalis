cmake_minimum_required(VERSION 3.20)
project(verbalis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VERBALIS_BUILD_TESTS "Build the C++ tests and the parity check" OFF)

file(GLOB_RECURSE VERBALIS_SOURCES CONFIGURE_DEPENDS src/verbalis/*.cpp)
add_library(verbalis STATIC ${VERBALIS_SOURCES})
add_library(verbalis::verbalis ALIAS verbalis)
target_include_directories(verbalis
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(VERBALIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/cpp ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
