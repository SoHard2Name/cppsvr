cmake_minimum_required(VERSION 3.10)
project(commlib)
set(CMAKE_CXX_STANDARD 17)
include_directories(${HEAD_PATH})

add_compile_options(-Werror)

aux_source_directory(./ SRC)
add_library(commlib ${SRC})


target_link_libraries(commlib ${YAMLCPP})

