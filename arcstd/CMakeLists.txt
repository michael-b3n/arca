cmake_minimum_required(VERSION 4.3)

project(arcstd)

#
# Define target sources
#
file(GLOB ARCSTD_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
file(GLOB ARCSTD_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.hpp")

#
# The private headers, all others are the interface. A private header refuses to be included without
# ARCSTD_INTERNAL, which only the sources of arcstd and the tests of its internals define.
#
set(ARCSTD_PRIVATE_INC_FILES
  cache/cache_database.hpp
  execution/execution.hpp
  lua/internal_scripts.hpp
  lua/names.hpp
  lua/sol_conversion.hpp
  lua/sol.hpp
  lua/state.hpp
  lua/state_owner.hpp
  setting/property_parser.hpp
  setting/property_tree.hpp
  web/web_request.hpp
)
list(TRANSFORM ARCSTD_PRIVATE_INC_FILES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(ARCSTD_PUBLIC_INC_FILES ${ARCSTD_INC_FILES} ${CMAKE_CURRENT_LIST_DIR}/export.hpp)
list(REMOVE_ITEM ARCSTD_PUBLIC_INC_FILES ${ARCSTD_PRIVATE_INC_FILES})

# The Lua scripts of lua/internal set up the state, a new one needs no other change.
include(embedded_scripts)
file(GLOB ARCSTD_LUA_INTERNAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/internal/*.lua")
configure_embedded_scripts(${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp
  HEADER arcstd/lua/internal_scripts.hpp
  NAMESPACE arcstd::lua::internal
  SCRIPTS ${ARCSTD_LUA_INTERNAL}
)
list(APPEND ARCSTD_SRC_FILES ${CMAKE_CURRENT_BINARY_DIR}/lua/internal_scripts.cpp)

find_package(Boost REQUIRED COMPONENTS filesystem url)
find_package(spdlog REQUIRED)
find_package(libzip REQUIRED)
find_package(sol2 3.5 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
# sol2 3.5 is made for Lua 5.1 to 5.4, the unversioned mingw package already ships 5.5.
pkg_search_module(LUA REQUIRED IMPORTED_TARGET lua5.4)
pkg_search_module(SQLITE REQUIRED IMPORTED_TARGET sqlite3)

#
# The libraries only the sources and the private headers need. A static arcstd passes them on to
# the link of its users, the header only ones are left out of the installed package, \see install.
#
set(ARCSTD_PRIVATE_LIBRARIES
  Boost::filesystem
  Boost::url
  spdlog::spdlog
  libzip::zip
  PkgConfig::LIBCURL
  PkgConfig::LUA
  PkgConfig::SQLITE
  $<BUILD_INTERFACE:sol2::sol2>
  $<BUILD_INTERFACE:beman::execution>
  $<BUILD_INTERFACE:rapidcsv::rapidcsv>
)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ARCSTD_INCLUDE_DIR)

#
# Define the library \p target of \p type from the sources of arcstd.
#
function(add_arcstd_library target type)
  add_library(${target} ${type})

  # The public headers are a header set. Its folder is the include path: the one above, for "arcstd/...".
  target_sources(${target}
    PRIVATE ${ARCSTD_SRC_FILES} ${ARCSTD_PRIVATE_INC_FILES}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${ARCSTD_INCLUDE_DIR}
    FILES ${ARCSTD_PUBLIC_INC_FILES}
  )
  target_compile_definitions(${target} PRIVATE ARCSTD_INTERNAL)
  target_compile_features(${target} PUBLIC cxx_std_26)

  # One export.hpp for both kinds, a static library tells it so.
  if(type STREQUAL "STATIC")
    target_compile_definitions(${target} PUBLIC ARCSTD_STATIC_DEFINE)
  endif()

  # Only what is marked ARCSTD_EXPORT leaves a shared library.
  set_target_properties(${target} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    DEFINE_SYMBOL arcstd_EXPORTS
  )

  # Link libraries, public are those the public headers need.
  target_link_libraries(${target}
    PUBLIC Boost::headers
    PUBLIC magic_enum::magic_enum
    PUBLIC stdc++exp
    PRIVATE ${ARCSTD_PRIVATE_LIBRARIES}
  )
  if(WIN32)
    target_link_libraries(${target} PRIVATE mswsock)
    target_compile_definitions(${target} PUBLIC NOMINMAX)
  endif()

  target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:ARCSTD_DEBUG>)
endfunction()

#
# The library, static unless ARCA_BUILD_SHARED is set.
#
if(ARCA_BUILD_SHARED)
  add_arcstd_library(${PROJECT_NAME} SHARED)
else()
  add_arcstd_library(${PROJECT_NAME} STATIC)
endif()
add_library(arca::arcstd ALIAS ${PROJECT_NAME})

# The target all_verify_interface_header_sets compiles each public header on its own: none may include a private one.
set_target_properties(${PROJECT_NAME} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

#
# Install arcstd with its public headers and the header only libraries they include, as package
# arca, \see tools/cmake/arca-config.cmake. Each kind has a targets file of its own, so a static
# and a shared build install into one prefix.
#
if(ARCA_INSTALL)
  include(GNUInstallDirs)
  if(ARCA_BUILD_SHARED)
    set(ARCSTD_KIND shared)
  else()
    set(ARCSTD_KIND static)
  endif()
  install(TARGETS ${PROJECT_NAME} magic_enum
    EXPORT arca_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(EXPORT arca_targets
    NAMESPACE arca::
    FILE arca-${ARCSTD_KIND}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/arca
  )
endif()

#
# arcstd_internals is arcstd for the tests of its internals: with the private headers and what they need. A shared
# library hides the internals, so it is a static one of its own then.
#
if(ARCA_BUILD_TESTS)
  add_library(arcstd_internals INTERFACE)
  if(ARCA_BUILD_SHARED)
    add_arcstd_library(arcstd_static STATIC)
    set_target_properties(arcstd_static PROPERTIES EXCLUDE_FROM_ALL ON)
    target_link_libraries(arcstd_internals INTERFACE arcstd_static)
  else()
    target_link_libraries(arcstd_internals INTERFACE ${PROJECT_NAME})
  endif()
  target_compile_definitions(arcstd_internals INTERFACE ARCSTD_INTERNAL)
  target_link_libraries(arcstd_internals INTERFACE ${ARCSTD_PRIVATE_LIBRARIES})
endif()
