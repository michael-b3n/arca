cmake_minimum_required(VERSION 4.3)
project(magic_enum VERSION 0.9.6)

# A header set, so the include path holds in the build and once installed with arcstd.
file(GLOB MAGIC_ENUM_INC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/magic_enum/*.hpp")
add_library(magic_enum INTERFACE)
target_sources(magic_enum INTERFACE FILE_SET HEADERS BASE_DIRS include FILES ${MAGIC_ENUM_INC_FILES})

add_library(magic_enum::magic_enum ALIAS magic_enum)
