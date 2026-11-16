#
# Package config of arca, installed next to the targets of arcstd. It provides arca::arcstd, static
# or shared, whichever was installed:
#
#   find_package(arca REQUIRED)                    # the kind BUILD_SHARED_LIBS asks for, else the other
#   find_package(arca REQUIRED COMPONENTS static)  # or shared, that kind only
#
# arca_SHARED_LIBS chooses the kind too, before BUILD_SHARED_LIBS. A shared arcstd is the DLL in
# bin, an application copies it next to itself, e.g. with $<TARGET_RUNTIME_DLLS:app>.
#
include(CMakeFindDependencyMacro)

#
# The kind asked for: by a component, arca_SHARED_LIBS or BUILD_SHARED_LIBS.
#
set(arca_kind "")
set(arca_kind_required FALSE)
foreach(arca_component IN LISTS arca_FIND_COMPONENTS)
  if(NOT arca_component MATCHES "^(static|shared)$")
    set(arca_FOUND FALSE)
    set(arca_NOT_FOUND_MESSAGE "arca has no component ${arca_component}, only static or shared")
    return()
  endif()
  if(arca_kind AND NOT arca_kind STREQUAL arca_component)
    set(arca_FOUND FALSE)
    set(arca_NOT_FOUND_MESSAGE "arca is either static or shared, not both")
    return()
  endif()
  set(arca_kind ${arca_component})
  set(arca_kind_required TRUE)
endforeach()
if(NOT arca_kind)
  if(DEFINED arca_SHARED_LIBS)
    set(arca_shared ${arca_SHARED_LIBS})
  else()
    set(arca_shared ${BUILD_SHARED_LIBS})
  endif()
  if(arca_shared)
    set(arca_kind shared)
  else()
    set(arca_kind static)
  endif()
endif()

# Without a component, the other kind serves if this one is not installed
set(arca_targets "${CMAKE_CURRENT_LIST_DIR}/arca-${arca_kind}-targets.cmake")
if(NOT EXISTS "${arca_targets}" AND NOT arca_kind_required)
  if(arca_kind STREQUAL "shared")
    set(arca_kind static)
  else()
    set(arca_kind shared)
  endif()
  set(arca_targets "${CMAKE_CURRENT_LIST_DIR}/arca-${arca_kind}-targets.cmake")
endif()
if(NOT EXISTS "${arca_targets}")
  set(arca_FOUND FALSE)
  set(arca_NOT_FOUND_MESSAGE "arca is not installed as ${arca_kind} library in ${CMAKE_CURRENT_LIST_DIR}")
  return()
endif()

#
# The libraries the targets name: the public headers need Boost, a static arcstd links all it uses.
#
if(arca_kind STREQUAL "static")
  find_dependency(Boost COMPONENTS filesystem url)
  find_dependency(spdlog)
  find_dependency(libzip)
  find_dependency(PkgConfig)
  if(NOT TARGET PkgConfig::LIBCURL)
    pkg_search_module(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
  endif()
  if(NOT TARGET PkgConfig::LUA)
    pkg_search_module(LUA REQUIRED IMPORTED_TARGET lua5.4)
  endif()
  if(NOT TARGET PkgConfig::SQLITE)
    pkg_search_module(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
  endif()
else()
  find_dependency(Boost)
endif()

include("${arca_targets}")
set(arca_${arca_kind}_FOUND TRUE)
