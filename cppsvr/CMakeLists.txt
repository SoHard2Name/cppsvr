
# 这个文件夹下主要放一些最通用的东西，后续开发都可以经常用到的，不止是 cppsvr。

cmake_minimum_required(VERSION 3.10)
project(commlib)
set(CMAKE_CXX_STANDARD 17)

# 注意是“_PATH”不是“_DIRECTORY”。。。
set(LIBRARY_OUTPUT_PATH ${LIB_PATH})

include_directories(${HEAD_PATH})

add_compile_options(-Werror)

enable_language(ASM)

aux_source_directory(./ SRC)
file(GLOB ASM_SOURCES "*.S")
list(APPEND SRC ${ASM_SOURCES})

# 因为一堆的 test 都在用这个库，所以它应该是弄成动态库比较合理
# 虽说几个小测试也没啥，但也是当做对于构建动态库的理解、实践
add_library(commlib SHARED ${SRC})

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)
target_link_libraries(commlib ${YAMLCPP} ${PTHREAD})

