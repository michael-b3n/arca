#
# Package config of arca, installed next to the targets of arcstd. It provides the static libraries
# arca::core, arca::io, arca::setting, arca::lua, arca::cache and arca::web, and arca::arcstd with
# all of them. They link the third party libraries they use, so those are found here.
#
include(CMakeFindDependencyMacro)

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

include("${CMAKE_CURRENT_LIST_DIR}/arca-targets.cmake")
