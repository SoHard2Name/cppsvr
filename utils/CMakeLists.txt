cmake_minimum_required(VERSION 3.10)
project(commlib)
set(CMAKE_CXX_STANDARD 14)
include_directories(${HEAD_PATH})

aux_source_directory(./ SRC)
add_library(commlib ${SRC})

find_library(YAMLCPP yaml-cpp)
target_link_libraries(commlib ${YAMLCPP})
