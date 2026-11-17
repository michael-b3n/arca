find_package(Catch2 3 REQUIRED)
include(Catch)

#
# The test helpers, for the tests of the projects built on arca too. Included by their path below
# this folder, e.g. "test_utils/temp_folder.hpp".
#
file(GLOB ARCSTD_TEST_UTILS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test_utils/*.hpp")
add_library(arcstd_test_utils INTERFACE)
target_sources(arcstd_test_utils INTERFACE ${ARCSTD_TEST_UTILS_FILES})
target_include_directories(arcstd_test_utils INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(arcstd_test_utils INTERFACE Catch2::Catch2 arca::arcstd)

#
# The tests of the interface, built like a project on arcstd: they include the public headers
# only.
#
file(GLOB ARCSTD_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
add_executable(arcstd_test ${ARCSTD_TEST_SRC_FILES})
target_link_libraries(arcstd_test PRIVATE Catch2::Catch2WithMain arcstd_test_utils arca::arcstd)

#
# The tests of the internals in the folder private, they include the private headers too, see
# arcstd_internals.
#
file(GLOB ARCSTD_PRIVATE_TEST_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/private/*/*.cpp")
add_executable(arcstd_private_test ${ARCSTD_PRIVATE_TEST_SRC_FILES})
target_include_directories(arcstd_private_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(arcstd_private_test PRIVATE Catch2::Catch2WithMain arcstd_internals)

catch_discover_tests(arcstd_test)
catch_discover_tests(arcstd_private_test)
