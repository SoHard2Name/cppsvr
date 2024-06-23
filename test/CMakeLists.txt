cmake_minimum_required(VERSION 3.10)
project(test)
set(CMAKE_CXX_STANDARD 14)
set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
include_directories(${HEAD_PATH})

add_executable(testlogger testlogger.cpp)
target_link_libraries(testlogger commlib)

add_executable(testyaml1 testyaml1.cpp)
find_library(YAMLCPP yaml-cpp)
target_link_libraries(testyaml1 commlib ${YAMLCPP})
