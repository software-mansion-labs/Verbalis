cmake_minimum_required(VERSION 3.21)
# The version of every package (Python reads it from here; ts/package.json must match, see tests/python/test_version.py)
project(verbalis VERSION 0.1.0 LANGUAGES CXX)

option(VERBALIS_BUILD_TESTS "Build the C++ tests and the parity check" OFF)
include(GNUInstallDirs)
option(VERBALIS_INSTALL "Install the library, its header, and a CMake package" ${PROJECT_IS_TOP_LEVEL})

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
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# The public header needs C++17; the sources need C++20. Set on the target, so a parent project keeps its own standard
target_compile_features(verbalis PUBLIC cxx_std_17 PRIVATE cxx_std_20)
set_target_properties(verbalis PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF)
# Warnings the code is kept free of; MSVC's flags differ. MSVC reads the UTF-8 sources only with /utf-8
target_compile_options(verbalis PRIVATE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -Wconversion>
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

if(VERBALIS_INSTALL)
  include(CMakePackageConfigHelpers)
  install(TARGETS verbalis EXPORT verbalisTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(EXPORT verbalisTargets NAMESPACE verbalis:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/verbalis)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/verbalisConfig.cmake
    "include(\${CMAKE_CURRENT_LIST_DIR}/verbalisTargets.cmake)\n")
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/verbalisConfigVersion.cmake
    COMPATIBILITY SameMinorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/verbalisConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/verbalisConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/verbalis)
endif()

if(VERBALIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/cpp ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()
