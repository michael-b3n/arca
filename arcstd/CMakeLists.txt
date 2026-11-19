#
# arcstd is a library per area, each links the ones it builds on: io and setting link core, lua
# links setting, cache and web link lua. core holds util, meta, math, signal, execution and system.
# A library is arcstd_<name>, arca::<name> for projects, and arca::arcstd links all of them.
#
include(arca-dependencies)
include(embedded_scripts)
# sol2 3.5 is made for Lua 5.1 to 5.4, arca-dependencies takes lua5.4: the unversioned MSYS2
# package already ships 5.5.
find_package(sol2 3.5 REQUIRED)

set(ARCSTD_TARGETS "")
set(ARCSTD_PRIVATE_LIBRARIES "")

#
# Define the static library arcstd_<name> of the folders AREAS, linking the arcstd libraries DEPENDS
# names and the PUBLIC_LIBRARIES. A header of PRIVATE_HEADERS refuses to be included without
# ARCSTD_INTERNAL, which only the sources of the library and the tests of the internals define.
# PRIVATE_LIBRARIES are what only the sources and the private headers need: the library passes them
# on to the link of its users, header only ones stay out of the installed package.
#
function(add_arcstd_library name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" ""
    "AREAS;PRIVATE_HEADERS;DEPENDS;PUBLIC_LIBRARIES;PRIVATE_LIBRARIES"
  )
  set(sources "")
  set(headers "")
  foreach(area IN LISTS arg_AREAS)
    file(GLOB area_sources CONFIGURE_DEPENDS ${area}/*.cpp)
    file(GLOB area_headers CONFIGURE_DEPENDS ${area}/*.hpp)
    list(APPEND sources ${area_sources})
    list(APPEND headers ${area_headers})
  endforeach()
  list(TRANSFORM arg_PRIVATE_HEADERS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
  if(arg_PRIVATE_HEADERS)
    list(REMOVE_ITEM headers ${arg_PRIVATE_HEADERS})
  endif()
  list(TRANSFORM arg_DEPENDS PREPEND arcstd_ OUTPUT_VARIABLE depends)

  set(target arcstd_${name})
  add_library(${target} STATIC)
  add_library(arca::${name} ALIAS ${target})
  target_sources(${target}
    PRIVATE ${sources} ${arg_PRIVATE_HEADERS}
    PUBLIC FILE_SET HEADERS BASE_DIRS ${PROJECT_SOURCE_DIR} FILES ${headers}
  )
  target_compile_features(${target} PUBLIC cxx_std_26)
  target_compile_definitions(${target}
    PRIVATE ARCSTD_INTERNAL $<$<CONFIG:Debug>:ARCSTD_DEBUG>
    PUBLIC $<$<PLATFORM_ID:Windows>:NOMINMAX>
  )
  target_link_libraries(${target}
    PUBLIC ${depends} ${arg_PUBLIC_LIBRARIES}
    PRIVATE ${arg_PRIVATE_LIBRARIES}
  )
  # The target all_verify_interface_header_sets compiles each public header on its own: none may
  # include a private one.
  set_target_properties(${target} PROPERTIES EXPORT_NAME ${name} VERIFY_INTERFACE_HEADER_SETS ON)

  set(ARCSTD_TARGETS ${ARCSTD_TARGETS} ${target} PARENT_SCOPE)
  set(ARCSTD_PRIVATE_LIBRARIES ${ARCSTD_PRIVATE_LIBRARIES} ${arg_PRIVATE_LIBRARIES} PARENT_SCOPE)
endfunction()

# stdc++exp holds std::stacktrace, which util::exception keeps.
add_arcstd_library(core
  AREAS util meta math signal execution system
  PRIVATE_HEADERS execution/execution.hpp
  PUBLIC_LIBRARIES Boost::headers magic_enum::magic_enum stdc++exp
  PRIVATE_LIBRARIES
    Boost::filesystem arca_external::spdlog $<BUILD_INTERFACE:beman::execution>
    $<$<PLATFORM_ID:Windows>:mswsock>
)

add_arcstd_library(io
  AREAS io
  DEPENDS core
  PRIVATE_LIBRARIES arca_external::zip $<BUILD_INTERFACE:rapidcsv::rapidcsv>
)

add_arcstd_library(setting
  AREAS setting
  PRIVATE_HEADERS setting/property_parser.hpp setting/property_tree.hpp
  DEPENDS core
)

add_arcstd_library(lua
  AREAS lua
  PRIVATE_HEADERS
    lua/internal_scripts.hpp lua/names.hpp lua/sol_conversion.hpp lua/sol.hpp lua/state.hpp
    lua/state_owner.hpp
  DEPENDS setting
  PRIVATE_LIBRARIES arca_external::lua $<BUILD_INTERFACE:sol2::sol2>
)
# The Lua scripts of lua/internal set up the state, a new one needs no other change.
file(GLOB ARCSTD_LUA_INTERNAL CONFIGURE_DEPENDS lua/internal/*.lua)
configure_embedded_scripts(${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp
  HEADER arcstd/lua/internal_scripts.hpp
  NAMESPACE arcstd::lua::internal
  SCRIPTS ${ARCSTD_LUA_INTERNAL}
)
target_sources(arcstd_lua PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp)

add_arcstd_library(cache
  AREAS cache
  PRIVATE_HEADERS cache/cache_database.hpp
  DEPENDS lua
  PRIVATE_LIBRARIES arca_external::sqlite
)

add_arcstd_library(web
  AREAS web
  PRIVATE_HEADERS web/web_request.hpp
  DEPENDS lua
  PRIVATE_LIBRARIES Boost::url arca_external::curl
)

add_library(arcstd INTERFACE)
add_library(arca::arcstd ALIAS arcstd)
target_link_libraries(arcstd INTERFACE ${ARCSTD_TARGETS})

#
# The package arca: the libraries with their public headers and magic_enum, which they include, and
# the package config, which finds the third party libraries they link.
#
if(ARCA_INSTALL)
  set(ARCA_PACKAGE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/arca)
  install(TARGETS ${ARCSTD_TARGETS} arcstd magic_enum EXPORT arca_targets FILE_SET HEADERS)
  install(EXPORT arca_targets
    NAMESPACE arca::
    FILE arca-targets.cmake
    DESTINATION ${ARCA_PACKAGE_DIR}
  )
  configure_file(${PROJECT_SOURCE_DIR}/tools/cmake/arca-config.cmake.in arca-config.cmake @ONLY)
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/arca-config.cmake
      ${PROJECT_SOURCE_DIR}/tools/cmake/arca-dependencies.cmake
    DESTINATION ${ARCA_PACKAGE_DIR}
  )
endif()

# arcstd for the tests of its internals, with the private headers and what they need
if(ARCA_BUILD_TESTS)
  add_library(arcstd_internals INTERFACE)
  target_compile_definitions(arcstd_internals INTERFACE ARCSTD_INTERNAL)
  target_link_libraries(arcstd_internals INTERFACE ${ARCSTD_TARGETS} ${ARCSTD_PRIVATE_LIBRARIES})
endif()
