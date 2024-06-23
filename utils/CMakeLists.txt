cmake_minimum_required(VERSION 3.10)
project(commlib)
set(CMAKE_CXX_STANDARD 17)

# 注意是“_PATH”不是“_DIRECTORY”。。。
set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

include_directories(${HEAD_PATH})

add_compile_options(-Werror)

aux_source_directory(./ SRC)
add_library(commlib ${SRC})

find_library(YAMLCPP yaml-cpp)
target_link_libraries(commlib ${YAMLCPP})

