cmake_minimum_required(VERSION 3.10)
project(test)
set(CMAKE_CXX_STANDARD 14)
set(EXECUTABLE_OUTPUT_PATH ${EXEC_PATH})
include_directories(${HEAD_PATH})

# 有了这一行以后下面就不用逐一去 target_link_libraries
link_libraries(commlib)

add_compile_options(-Werror)

add_executable(testlogger testlogger.cpp)
add_executable(testyaml1 testyaml1.cpp)
add_executable(testyaml2 testyaml2.cpp)
add_executable(testthread testthread.cpp)
add_executable(testmutex testmutex.cpp)
add_executable(testcontext testcontext.cpp)
add_executable(testfiber testfiber.cpp)
add_executable(testscheduler testscheduler.cpp)
add_executable(testasynlogger testasynlogger.cpp)

add_executable(studyscheduler studyscheduler.cpp)