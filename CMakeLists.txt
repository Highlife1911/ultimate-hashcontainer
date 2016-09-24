cmake_minimum_required(VERSION 3.2)

PROJECT(ultimate-hashcontainer)

include(cmake/init.cmake)
include(cmake/gtest.cmake)

init_project()

add_gtest()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(tests)
