find_package(Catch2 3 REQUIRED)
include(Catch)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml)

#
# The tests of the C++ types of ArcQml that need no window. They run under the QCoreApplication of
# main.cpp, whose event loop takes the notifications the types post.
#
file(GLOB ARCQML_TEST_SRC_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp"
)
add_executable(arcqml_test ${ARCQML_TEST_SRC_FILES})
target_include_directories(arcqml_test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(arcqml_test PRIVATE
  Catch2::Catch2 arcstd_test_utils ArcQml Qt6::Core Qt6::Gui Qt6::Qml
)

# ctest lists the tests before it runs them. The Qt DLLs are found by the listing and by the tests,
# also with no Qt on the PATH.
catch_discover_tests(arcqml_test
  DISCOVERY_MODE PRE_TEST
  DL_PATHS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
)
