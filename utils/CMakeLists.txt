cmake_minimum_required(VERSION 3.10)
project(commlib)
set(CMAKE_CXX_STANDARD 14)
set(LIBRARY_OUTPUT_PATH ${LIB_PATH})
include_directories(${HEAD_PATH})

aux_source_directory(./ SRC)
add_library(commlib ${SRC})
