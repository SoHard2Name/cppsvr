cmake_minimum_required(VERSION 3.10)
project(cppsvr_test)
set(CMAKE_CXX_STANDARD 14)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cppsvr/include)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(cppsvr)
add_subdirectory(test)
