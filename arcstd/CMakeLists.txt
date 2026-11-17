#
# arcstd is a library per area, each links the ones it builds on: io and setting link core, lua
# links setting, cache and web link lua. core holds util, meta, math, signal, execution and system.
# A library is arcstd_<name>, arca::<name> for projects, and arca::arcstd links all of them.
#
find_package(Boost REQUIRED COMPONENTS filesystem url)
find_package(spdlog REQUIRED)
find_package(libzip REQUIRED)
find_package(sol2 3.5 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
# sol2 3.5 is made for Lua 5.1 to 5.4, the unversioned MSYS2 package already ships 5.5.
pkg_search_module(LUA REQUIRED IMPORTED_TARGET lua5.4)
pkg_search_module(SQLITE REQUIRED IMPORTED_TARGET sqlite3)

include(embedded_scripts)

# The public headers are included as "arcstd/...", from the folder above.
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ARCSTD_INCLUDE_DIR)

# A shared arcstd hides the internals, the tests of the internals link static libraries of their own.
if(ARCA_BUILD_SHARED)
  set(ARCSTD_KIND SHARED)
else()
  set(ARCSTD_KIND STATIC)
endif()
set(ARCSTD_LIBRARIES "")

#
# Define the library arcstd_<name> of the folders AREAS, linking the arcstd libraries DEPENDS names
# and the PUBLIC_LIBRARIES. A header of PRIVATE_HEADERS, relative to arcstd, refuses to be included
# without ARCSTD_INTERNAL, which only the sources of the library and the tests of the internals
# define. PRIVATE_LIBRARIES are what only the sources and the private headers need: a static
# library passes them on to the link of its users, header only ones stay out of the installed
# package. SOURCES are generated sources the library compiles too.
#
function(add_arcstd_library name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" ""
    "AREAS;PRIVATE_HEADERS;DEPENDS;PUBLIC_LIBRARIES;PRIVATE_LIBRARIES;SOURCES"
  )
  set(sources ${arg_SOURCES})
  set(headers "")
  foreach(area IN LISTS arg_AREAS)
    file(GLOB area_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${area}/*.cpp")
    file(GLOB area_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${area}/*.hpp")
    list(APPEND sources ${area_sources})
    list(APPEND headers ${area_headers})
  endforeach()
  list(TRANSFORM arg_PRIVATE_HEADERS PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
  if(headers AND arg_PRIVATE_HEADERS)
    list(REMOVE_ITEM headers ${arg_PRIVATE_HEADERS})
  endif()
  if(name STREQUAL "core")
    list(APPEND headers ${CMAKE_CURRENT_LIST_DIR}/export.hpp)
  endif()

  set(kinds ${ARCSTD_KIND})
  if(ARCA_BUILD_TESTS AND ARCA_BUILD_SHARED)
    list(APPEND kinds INTERNALS)
  endif()
  foreach(kind IN LISTS kinds)
    if(kind STREQUAL "INTERNALS")
      set(suffix _internals)
      set(type STATIC)
    else()
      set(suffix "")
      set(type ${kind})
    endif()
    set(target arcstd_${name}${suffix})
    list(TRANSFORM arg_DEPENDS PREPEND arcstd_ OUTPUT_VARIABLE depends)
    list(TRANSFORM depends APPEND "${suffix}")

    add_library(${target} ${type})
    target_sources(${target}
      PRIVATE ${sources} ${arg_PRIVATE_HEADERS}
      PUBLIC FILE_SET HEADERS BASE_DIRS ${ARCSTD_INCLUDE_DIR} FILES ${headers}
    )
    target_compile_features(${target} PUBLIC cxx_std_26)
    target_compile_definitions(${target}
      PRIVATE ARCSTD_INTERNAL $<$<CONFIG:Debug>:ARCSTD_DEBUG>
      PUBLIC $<$<PLATFORM_ID:Windows>:NOMINMAX> $<$<STREQUAL:${type},STATIC>:ARCSTD_STATIC_DEFINE>
    )
    target_link_libraries(${target}
      PUBLIC ${depends} ${arg_PUBLIC_LIBRARIES}
      PRIVATE ${arg_PRIVATE_LIBRARIES}
    )
    # Only what is marked ARCSTD_<NAME>_EXPORT leaves a shared library.
    set_target_properties(${target} PROPERTIES
      CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
      DEFINE_SYMBOL arcstd_${name}_EXPORTS
      EXPORT_NAME ${name}
    )
    if(kind STREQUAL "INTERNALS")
      set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL ON)
    else()
      # The target all_verify_interface_header_sets compiles each public header on its own: none
      # may include a private one.
      set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
      add_library(arca::${name} ALIAS ${target})
    endif()
  endforeach()

  set(ARCSTD_LIBRARIES ${ARCSTD_LIBRARIES} ${name} PARENT_SCOPE)
  set(ARCSTD_${name}_PRIVATE_LIBRARIES ${arg_PRIVATE_LIBRARIES} PARENT_SCOPE)
endfunction()

# stdc++exp holds std::stacktrace, which util::exception keeps.
add_arcstd_library(core
  AREAS util meta math signal execution system
  PRIVATE_HEADERS execution/execution.hpp
  PUBLIC_LIBRARIES Boost::headers magic_enum::magic_enum stdc++exp
  PRIVATE_LIBRARIES
    Boost::filesystem spdlog::spdlog $<BUILD_INTERFACE:beman::execution>
    $<$<PLATFORM_ID:Windows>:mswsock>
)

add_arcstd_library(io
  AREAS io
  DEPENDS core
  PRIVATE_LIBRARIES libzip::zip $<BUILD_INTERFACE:rapidcsv::rapidcsv>
)

add_arcstd_library(setting
  AREAS setting
  PRIVATE_HEADERS setting/property_parser.hpp setting/property_tree.hpp
  DEPENDS core
)

# The Lua scripts of lua/internal set up the state, a new one needs no other change.
file(GLOB ARCSTD_LUA_INTERNAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/internal/*.lua")
configure_embedded_scripts(${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp
  HEADER arcstd/lua/internal_scripts.hpp
  NAMESPACE arcstd::lua::internal
  SCRIPTS ${ARCSTD_LUA_INTERNAL}
)
add_arcstd_library(lua
  AREAS lua
  PRIVATE_HEADERS
    lua/internal_scripts.hpp lua/names.hpp lua/sol_conversion.hpp lua/sol.hpp lua/state.hpp
    lua/state_owner.hpp
  DEPENDS setting
  PRIVATE_LIBRARIES PkgConfig::LUA $<BUILD_INTERFACE:sol2::sol2>
  SOURCES ${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp
)

add_arcstd_library(cache
  AREAS cache
  PRIVATE_HEADERS cache/cache_database.hpp
  DEPENDS lua
  PRIVATE_LIBRARIES PkgConfig::SQLITE
)

add_arcstd_library(web
  AREAS web
  PRIVATE_HEADERS web/web_request.hpp
  DEPENDS lua
  PRIVATE_LIBRARIES Boost::url PkgConfig::LIBCURL
)

add_library(arcstd INTERFACE)
list(TRANSFORM ARCSTD_LIBRARIES PREPEND arcstd_ OUTPUT_VARIABLE ARCSTD_TARGETS)
target_link_libraries(arcstd INTERFACE ${ARCSTD_TARGETS})
add_library(arca::arcstd ALIAS arcstd)

#
# Install the libraries with their public headers and magic_enum, which they include, as package
# arca, see tools/cmake/arca-config.cmake. Each kind has a targets file of its own, so a static and
# a shared build install into one prefix.
#
if(ARCA_INSTALL)
  include(GNUInstallDirs)
  string(TOLOWER ${ARCSTD_KIND} ARCSTD_KIND_NAME)
  install(TARGETS ${ARCSTD_TARGETS} arcstd magic_enum
    EXPORT arca_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(EXPORT arca_targets
    NAMESPACE arca::
    FILE arca-${ARCSTD_KIND_NAME}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arca
  )
endif()

#
# arcstd_internals is arcstd for the tests of its internals: with the private headers and what they
# need, static also in a shared build.
#
if(ARCA_BUILD_TESTS)
  add_library(arcstd_internals INTERFACE)
  target_compile_definitions(arcstd_internals INTERFACE ARCSTD_INTERNAL)
  foreach(name IN LISTS ARCSTD_LIBRARIES)
    if(ARCA_BUILD_SHARED)
      target_link_libraries(arcstd_internals INTERFACE arcstd_${name}_internals)
    else()
      target_link_libraries(arcstd_internals INTERFACE arcstd_${name})
    endif()
    target_link_libraries(arcstd_internals INTERFACE ${ARCSTD_${name}_PRIVATE_LIBRARIES})
  endforeach()
endif()
