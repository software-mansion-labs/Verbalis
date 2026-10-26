# Unit tests of the C++ core, and the parity check against tests/parity/baseline.
file(GLOB VERBALIS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)
add_executable(verbalis_tests ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp ${VERBALIS_TEST_SOURCES})
target_include_directories(verbalis_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(verbalis_tests PRIVATE verbalis)
add_test(NAME unit COMMAND verbalis_tests)

add_executable(verbalis_parity ${CMAKE_CURRENT_SOURCE_DIR}/parity.cpp)
target_include_directories(verbalis_parity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(verbalis_parity PRIVATE verbalis)
add_test(NAME parity COMMAND verbalis_parity ${CMAKE_CURRENT_SOURCE_DIR}/../parity/baseline)
