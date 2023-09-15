cmake_minimum_required(VERSION 2.5)
project(tinyMuduo)

# 设置tinyMuduo最终编译成动态库的路径，动态库存放在根目录下的lib目录下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置编译选项，其中-g表示生成调试符号，-std=c++11表示启用c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
# 定义参与编译的源文件
aux_source_directory(. SRC_LIST)

# 生成动态库的名称
add_library(tinyMuduo SHARED ${SRC_LIST})

