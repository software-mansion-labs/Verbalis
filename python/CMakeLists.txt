# The Python extension verbalis._verbalis, built by scikit-build-core from the repository's pyproject.toml.
cmake_minimum_required(VERSION 3.20)
project(verbalis_python LANGUAGES CXX)

find_package(Python 3.9 REQUIRED COMPONENTS Interpreter Development.Module ${SKBUILD_SABI_COMPONENT})
find_package(nanobind CONFIG REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cpp ${CMAKE_CURRENT_BINARY_DIR}/cpp)

nanobind_add_module(_verbalis STABLE_ABI NB_STATIC src/bindings.cpp)
target_link_libraries(_verbalis PRIVATE verbalis::verbalis)

nanobind_add_stub(_verbalis_stub
  MODULE _verbalis
  OUTPUT _verbalis.pyi
  PYTHON_PATH $<TARGET_FILE_DIR:_verbalis>
  DEPENDS _verbalis)

install(TARGETS _verbalis LIBRARY DESTINATION verbalis)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_verbalis.pyi DESTINATION verbalis)
