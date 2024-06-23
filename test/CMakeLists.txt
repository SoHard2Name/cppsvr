cmake_minimum_required(VERSION 3.10)
project(test)
set(CMAKE_CXX_STANDARD 14)
set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
include_directories(${HEAD_PATH})

# 有了这一行以后下面就不用逐一去 target_link_libraries
link_libraries(commlib)

add_executable(testlogger testlogger.cpp)
add_executable(testyaml1 testyaml1.cpp)
add_executable(testyaml2 testyaml2.cpp)
add_executable(testthread testthread.cpp)
add_executable(testmutex testmutex.cpp)

