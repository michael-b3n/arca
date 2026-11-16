cmake_minimum_required(VERSION 4.3)
project(incbin VERSION 1.0.0)

# A header set, so the include path holds in the build and once installed with arcstd.
add_library(incbin INTERFACE)
target_sources(incbin INTERFACE FILE_SET HEADERS BASE_DIRS include FILES include/incbin.h)

add_library(incbin::incbin ALIAS incbin)
