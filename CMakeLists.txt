cmake_minimum_required(VERSION 4.3)

#
# Set global CMake variables.
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")

#
# Limit CMake to only provide "Release" and "Debug" others are not needed, when arca is built on its own.
# see https://cmake.org/cmake/help/latest/variable/CMAKE_CONFIGURATION_TYPES.html
#
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_CONFIGURATION_TYPES Release Debug)
endif()

project(arca)

#
# Settings of a build of arca on its own, a project including arca brings its own.
#
if(PROJECT_IS_TOP_LEVEL)
  # An empty build type builds without optimization and debug info.
  get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set, configure with a preset, e.g. `cmake --preset gcc-release --fresh`")
  endif()

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_CXX_STANDARD 26)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  add_compile_options(-Wall -Wpedantic)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

  # Source paths in __FILE__ and std::source_location are kept relative.
  add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=>)

  # GNU ld overflows REL32 relocations linking clang objects against the GCC-built mingw libraries.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_LINKER_TYPE LLD)
  endif()

  enable_testing()
endif()

#
# Main Qt Versions, a project including arca may choose others.
#
if(NOT DEFINED QT_VERSION_NAME)
  set(QT_VERSION_NAME Qt6)
endif()
if(NOT DEFINED QT_VERSION_NUMBER)
  set(QT_VERSION_NUMBER 6.11)
endif()

#
# The tests and their helpers, which the tests of a project built on arca may use too.
#
option(ARCA_BUILD_TESTS "Build the tests of arca and arcstd_test_utils" ${PROJECT_IS_TOP_LEVEL})

#
# Include all subdirectories.
#
add_subdirectory(libs_external)

add_compile_options(-Wswitch-enum)

add_subdirectory(arcstd)
add_subdirectory(arcqml)
if(ARCA_BUILD_TESTS)
  add_subdirectory(arcstd_test)
endif()
