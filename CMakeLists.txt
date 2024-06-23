cmake_minimum_required(VERSION 3.10)
project(cppsvr)
set(CMAKE_CXX_STANDARD 14)

set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(utils)
add_subdirectory(test)

find_library(YAMLCPP yaml-cpp)
