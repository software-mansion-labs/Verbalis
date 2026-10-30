cmake_minimum_required(VERSION 3.20)
project(verbalis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VERBALIS_BUILD_TESTS "Build the C++ tests and the parity check" OFF)

add_library(verbalis STATIC
  src/verbalis/languages/de/german.cpp
  src/verbalis/languages/en/english.cpp
  src/verbalis/languages/es/spanish.cpp
  src/verbalis/languages/fr/french.cpp
  src/verbalis/languages/hi/hindi.cpp
  src/verbalis/languages/it/italian.cpp
  src/verbalis/languages/language.cpp
  src/verbalis/languages/pl/polish.cpp
  src/verbalis/languages/pt/portuguese.cpp
  src/verbalis/languages/registry.cpp
  src/verbalis/languages/zh/mandarin.cpp
  src/verbalis/numbers/patterns.cpp
  src/verbalis/numbers/scanning.cpp
  src/verbalis/steps/normalize.cpp
  src/verbalis/steps/symbols.cpp
  src/verbalis/steps/terms.cpp
  src/verbalis/steps/trim.cpp
  src/verbalis/text/context.cpp
  src/verbalis/text/document.cpp
  src/verbalis/text/readings.cpp
  src/verbalis/text/scan.cpp
  src/verbalis/text/utf8.cpp
  src/verbalis/verbalizer.cpp)
add_library(verbalis::verbalis ALIAS verbalis)
target_include_directories(verbalis
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Warnings the code is kept free of; MSVC's flags differ
target_compile_options(verbalis PRIVATE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion>)

if(VERBALIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/cpp ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
