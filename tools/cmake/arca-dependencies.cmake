#
# The third party libraries the libraries of arcstd pass on to the link, as the targets
# arca_external::<name>. The build of arca includes this file and so does the installed package
# config, which sets ARCA_FIND_AS_DEPENDENCY. With ARCA_STATIC_DEPENDENCIES they are the static
# libraries of MSYS2 with all they link, so an application linked with -static needs no DLL of them.
# spdlog has no static library there, it is header only then.
#
if(TARGET arca_external::spdlog)
  return()
endif()

macro(arca_find_package)
  if(ARCA_FIND_AS_DEPENDENCY)
    find_dependency(${ARGN})
  else()
    find_package(${ARGN} REQUIRED)
  endif()
endmacro()

if(ARCA_STATIC_DEPENDENCIES)
  set(Boost_USE_STATIC_LIBS ON)
endif()
arca_find_package(Boost COMPONENTS filesystem url)

add_library(arca_external::spdlog INTERFACE IMPORTED)
if(ARCA_STATIC_DEPENDENCIES)
  # COMPONENTS static would fail on fmt's check of its components
  set(fmt_SHARED_LIBS OFF)
  arca_find_package(fmt)
  arca_find_package(spdlog)
  target_link_libraries(arca_external::spdlog INTERFACE spdlog::spdlog_header_only)
else()
  arca_find_package(spdlog)
  target_link_libraries(arca_external::spdlog INTERFACE spdlog::spdlog)
endif()

#
# A library pkg-config knows as arca_external::<name>. Static, it links the libraries pkg-config
# names with --static and takes its static compile flags, e.g. CURL_STATICLIB.
#
arca_find_package(PkgConfig)
if(ARCA_FIND_AS_DEPENDENCY)
  set(arca_pkg_required "")
else()
  set(arca_pkg_required REQUIRED)
endif()
foreach(entry IN ITEMS "curl=libcurl" "lua=lua5.4" "sqlite=sqlite3" "zip=libzip")
  string(REPLACE "=" ";" entry ${entry})
  list(GET entry 0 name)
  list(GET entry 1 module)
  string(TOUPPER "ARCA_EXTERNAL_${name}" prefix)
  if(ARCA_STATIC_DEPENDENCIES)
    pkg_search_module(${prefix} ${arca_pkg_required} ${module})
  else()
    pkg_search_module(${prefix} ${arca_pkg_required} IMPORTED_TARGET ${module})
  endif()
  # Found as dependency of the package, a missing library is a missing arca, as for find_dependency
  if(NOT ${prefix}_FOUND)
    set(arca_FOUND FALSE)
    set(arca_NOT_FOUND_MESSAGE "arca needs ${module}, pkg-config does not find it")
    return()
  endif()
  add_library(arca_external::${name} INTERFACE IMPORTED)
  if(ARCA_STATIC_DEPENDENCIES)
    set_target_properties(arca_external::${name} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${${prefix}_STATIC_INCLUDE_DIRS}"
      INTERFACE_COMPILE_OPTIONS "${${prefix}_STATIC_CFLAGS_OTHER}"
      INTERFACE_LINK_DIRECTORIES "${${prefix}_STATIC_LIBRARY_DIRS}"
      INTERFACE_LINK_LIBRARIES "${${prefix}_STATIC_LIBRARIES}"
    )
  else()
    target_link_libraries(arca_external::${name} INTERFACE PkgConfig::${prefix})
  endif()
endforeach()
