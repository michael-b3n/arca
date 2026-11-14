cmake_minimum_required(VERSION 4.3)

project(arcstd_test LANGUAGES CXX)

find_package(Catch2 3 REQUIRED)
include(Catch)

#
# The test helpers, shared with the tests of the libraries built on arcstd.
# Includable by their path below the test root, e.g. "test_utils/temp_folder.hpp".
#
file(GLOB ARCSTD_TEST_UTILS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/*.hpp")

add_library(arcstd_test_utils INTERFACE)
target_sources(arcstd_test_utils
  INTERFACE ${ARCSTD_TEST_UTILS_FILES}
)
target_include_directories(arcstd_test_utils
  INTERFACE ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(arcstd_test_utils
  INTERFACE Catch2::Catch2
  INTERFACE arcstd
)

#
# The tests of the interface, built like a project on arcstd: they include the public headers only and link the
# library as it is, static or shared.
#
file(GLOB ARCSTD_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
list(FILTER ARCSTD_TEST_SRC_FILES EXCLUDE REGEX "/test_utils/")

add_executable(arcstd_test)
target_sources(arcstd_test
  PRIVATE ${ARCSTD_TEST_SRC_FILES}
)
target_link_libraries(
  arcstd_test
  PRIVATE
  Catch2::Catch2WithMain
  arcstd_test_utils
  arcstd)

# Test discovery runs the tests after the build, they need the library beside them.
if(ARCA_BUILD_SHARED)
  add_custom_command(TARGET arcstd_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:arcstd> $<TARGET_FILE_DIR:arcstd_test>
  )
endif()

#
# The tests of the internals in the folder private, they include the private headers too, \see arcstd_internals.
#
file(GLOB ARCSTD_PRIVATE_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/private/*/*.cpp")

add_executable(arcstd_private_test)
target_sources(arcstd_private_test
  PRIVATE ${ARCSTD_PRIVATE_TEST_SRC_FILES}
)
target_include_directories(arcstd_private_test
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(
  arcstd_private_test
  PRIVATE
  Catch2::Catch2WithMain
  arcstd_internals)

#
# Register the Catch2 test cases with CTest, so they can be run via `ctest` in the build directory.
#
catch_discover_tests(arcstd_test)
catch_discover_tests(arcstd_private_test)
