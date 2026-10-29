# The Python extension verbalis._verbalis, built by scikit-build-core from the repository's pyproject.toml. Its stub,
# verbalis/_verbalis.pyi, is committed rather than generated here: generating it imports the built module, which fails
# when cross-compiling. tests/python/test_stub.py checks that it matches the module.
cmake_minimum_required(VERSION 3.20)
project(verbalis_python LANGUAGES CXX)

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module ${SKBUILD_SABI_COMPONENT})
find_package(nanobind CONFIG REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../cpp ${CMAKE_CURRENT_BINARY_DIR}/cpp)

nanobind_add_module(_verbalis STABLE_ABI NB_STATIC src/bindings.cpp)
target_link_libraries(_verbalis PRIVATE verbalis::verbalis)

install(TARGETS _verbalis LIBRARY DESTINATION verbalis)
