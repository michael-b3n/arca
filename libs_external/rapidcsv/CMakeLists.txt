cmake_minimum_required(VERSION 4.3)

project(rapidcsv VERSION 9.7.0 LANGUAGES CXX)

add_library(rapidcsv INTERFACE)
# SYSTEM, so the warnings of arca stay out of the third party header
target_include_directories(rapidcsv SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_library(rapidcsv::rapidcsv ALIAS rapidcsv)
