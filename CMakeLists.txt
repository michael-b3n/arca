cmake_minimum_required(VERSION 4.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tools/cmake")

# Built on its own, arca offers Release and Debug only, a project including it brings its own.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_CONFIGURATION_TYPES Release Debug)
endif()

project(arca LANGUAGES C CXX)

#
# Settings of a build of arca on its own, a project including arca brings its own.
#
if(PROJECT_IS_TOP_LEVEL)
  # An empty build type would build without optimization and debug info.
  get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set, configure with a preset, e.g. `cmake --preset gcc-release --fresh`")
  endif()

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  # The executables find the libraries of a shared build beside them.
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_CXX_STANDARD 26)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  # arca has no modules, the scan for them adds GCC flags clang-tidy does not know.
  set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

  add_compile_options(-Wall -Wpedantic)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

  # Source paths in __FILE__ and std::source_location stay relative.
  add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=>)

  # GNU ld overflows REL32 relocations linking clang objects against the GCC-built mingw libraries.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_LINKER_TYPE LLD)
  endif()

  enable_testing()
endif()

# The Qt of arcqml, a project including arca may choose another.
set(QT_VERSION_NAME Qt6 CACHE STRING "Qt major version of arcqml, e.g. Qt6")
set(QT_VERSION_NUMBER 6.11 CACHE STRING "Lowest Qt version arcqml takes")

option(ARCA_BUILD_TESTS "Build the tests of arca and arcstd_test_utils" ${PROJECT_IS_TOP_LEVEL})

# One kind per build, a static and a shared build install side by side. arcqml is always static.
option(ARCA_BUILD_SHARED "Build arcstd as a shared library" ${BUILD_SHARED_LIBS})

# For a project that finds arca with find_package(arca). One that includes it with
# add_subdirectory uses it from the build.
option(ARCA_INSTALL "Install arcstd as the CMake package arca" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(libs_external)

# Below the external libraries, their switches need not name every value.
add_compile_options(-Wswitch-enum)

add_subdirectory(arcstd)
add_subdirectory(arcqml)
if(ARCA_BUILD_TESTS)
  add_subdirectory(arcstd_test)
endif()

if(ARCA_INSTALL)
  include(GNUInstallDirs)
  install(FILES tools/cmake/arca-config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arca)
endif()
