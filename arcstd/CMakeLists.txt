cmake_minimum_required(VERSION 4.3)

project(arcstd)

#
# Define target sources
#
file(GLOB ARCSTD_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.cpp")
file(GLOB ARCSTD_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/*.hpp")

# Include OS specific sources and headers
if(WIN32)
  file(GLOB ARCSTD_WINDOWS_SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.cpp")
  file(GLOB ARCSTD_WINDOWS_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*/windows/*.hpp")
  list(APPEND ARCSTD_SRC_FILES ${ARCSTD_WINDOWS_SRC_FILES})
  list(APPEND ARCSTD_INC_FILES ${ARCSTD_WINDOWS_INC_FILES})
endif()

#
# The private headers, all others are the interface. A private header refuses to be included without
# ARCSTD_INTERNAL, which only the sources of arcstd and the tests of its internals define.
#
set(ARCSTD_PRIVATE_INC_FILES
  cache/cache_database.hpp
  execution/execution.hpp
  lua/internal_scripts.hpp
  lua/names.hpp
  lua/object_cast.hpp
  lua/sol.hpp
  lua/state.hpp
  lua/state_owner.hpp
  setting/property_parser.hpp
  setting/property_tree.hpp
  web/web_request.hpp
)
list(TRANSFORM ARCSTD_PRIVATE_INC_FILES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")
set(ARCSTD_PUBLIC_INC_FILES ${ARCSTD_INC_FILES})
list(REMOVE_ITEM ARCSTD_PUBLIC_INC_FILES ${ARCSTD_PRIVATE_INC_FILES})

#
# Compile the Lua scripts of lua/internal into the library, a new script needs no other change.
# The label is the namespace of the scripts, \see lua/internal_scripts.hpp.
#
include(incbin)
file(GLOB ARCSTD_LUA_INTERNAL CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/lua/internal/*.lua")
set(INCBIN_HEADER arcstd/lua/internal_scripts.hpp)
set(INCBIN_NAMESPACE arcstd::lua::internal)
configure_incbin_source(internal ${PROJECT_SOURCE_DIR}/../tools/cmake/embedded_scripts.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp ${ARCSTD_LUA_INTERNAL})
list(APPEND ARCSTD_SRC_FILES ${CMAKE_CURRENT_BINARY_DIR}/lua/internal.cpp)

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
# The libraries only the sources and the private headers need.
#
set(ARCSTD_PRIVATE_LIBRARIES
  Boost::filesystem
  Boost::url
  spdlog::spdlog
  libzip::zip
  PkgConfig::LIBCURL
  PkgConfig::LUA
  PkgConfig::SQLITE
  sol2::sol2
  beman::execution
  rapidcsv::rapidcsv
)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ARCSTD_INCLUDE_DIR)

#
# Define the library \p target of \p type from the sources of arcstd.
#
function(add_arcstd_library target type)
  add_library(${target} ${type})

  # The public headers are header sets. Their folders are the include paths: the one above, for "arcstd/...".
  # The generated header is a set of its own, the build folder may lie inside the other one.
  target_sources(${target}
    PRIVATE ${ARCSTD_SRC_FILES} ${ARCSTD_PRIVATE_INC_FILES}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${ARCSTD_INCLUDE_DIR}
    FILES ${ARCSTD_PUBLIC_INC_FILES}
    PUBLIC FILE_SET generated TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/arcstd/export.hpp
  )
  target_compile_definitions(${target} PRIVATE ARCSTD_INTERNAL)

  # Only what is marked ARCSTD_EXPORT leaves a shared library.
  set_target_properties(${target} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    DEFINE_SYMBOL arcstd_EXPORTS
  )

  # Link libraries, public are those the public headers need.
  target_link_libraries(${target}
    PUBLIC Boost::headers
    PUBLIC incbin::incbin
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
# ARCSTD_EXPORT, written for the kind of library arcstd is.
#
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/arcstd/export.hpp
  EXPORT_MACRO_NAME ARCSTD_EXPORT
  NO_EXPORT_MACRO_NAME ARCSTD_NO_EXPORT
  STATIC_DEFINE ARCSTD_STATIC_DEFINE
)

#
# arcstd_internals is arcstd for the tests of its internals: with the private headers and what they need. A shared
# library hides the internals, so it is a static one of its own then.
#
if(ARCA_BUILD_TESTS)
  add_library(arcstd_internals INTERFACE)
  if(ARCA_BUILD_SHARED)
    add_arcstd_library(arcstd_static STATIC)
    target_compile_definitions(arcstd_static PUBLIC ARCSTD_STATIC_DEFINE)
    set_target_properties(arcstd_static PROPERTIES EXCLUDE_FROM_ALL ON)
    target_link_libraries(arcstd_internals INTERFACE arcstd_static)
  else()
    target_link_libraries(arcstd_internals INTERFACE ${PROJECT_NAME})
  endif()
  target_compile_definitions(arcstd_internals INTERFACE ARCSTD_INTERNAL)
  target_link_libraries(arcstd_internals INTERFACE ${ARCSTD_PRIVATE_LIBRARIES})
endif()
