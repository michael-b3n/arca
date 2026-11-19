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

if(ARCA_FIND_AS_DEPENDENCY)
  set(arca_required "")
else()
  set(arca_required REQUIRED)
endif()

#
# Find the package ARGN. The macros return from this file, so a missing library ends the package
# config the way find_dependency does.
#
macro(arca_find_package)
  if(ARCA_FIND_AS_DEPENDENCY)
    find_dependency(${ARGN})
  else()
    find_package(${ARGN} REQUIRED)
  endif()
endmacro()

#
# Find the library <module> with pkg-config as arca_external::<name>. Static, it links what
# pkg-config --static names and takes its static compile flags, e.g. CURL_STATICLIB.
#
macro(arca_find_pkg_config name module)
  string(TOUPPER "arca_external_${name}" arca_prefix)
  if(ARCA_STATIC_DEPENDENCIES)
    pkg_search_module(${arca_prefix} ${arca_required} ${module})
  else()
    pkg_search_module(${arca_prefix} ${arca_required} IMPORTED_TARGET ${module})
  endif()
  if(NOT ${arca_prefix}_FOUND)
    set(arca_FOUND FALSE)
    set(arca_NOT_FOUND_MESSAGE "arca needs ${module}, pkg-config does not find it")
    return()
  endif()

  add_library(arca_external::${name} INTERFACE IMPORTED)
  if(ARCA_STATIC_DEPENDENCIES)
    set_target_properties(arca_external::${name} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${${arca_prefix}_STATIC_INCLUDE_DIRS}"
      INTERFACE_COMPILE_OPTIONS "${${arca_prefix}_STATIC_CFLAGS_OTHER}"
      INTERFACE_LINK_DIRECTORIES "${${arca_prefix}_STATIC_LIBRARY_DIRS}"
      INTERFACE_LINK_LIBRARIES "${${arca_prefix}_STATIC_LIBRARIES}"
    )
  else()
    target_link_libraries(arca_external::${name} INTERFACE PkgConfig::${arca_prefix})
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

arca_find_package(PkgConfig)
arca_find_pkg_config(curl libcurl)
arca_find_pkg_config(lua lua5.4)
arca_find_pkg_config(sqlite sqlite3)
arca_find_pkg_config(zip libzip)
