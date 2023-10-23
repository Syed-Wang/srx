cmake_minimum_required(VERSION 3.15) # CMake 最低版本号要求
project(dev_syn) # 项目信息

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -w") # 设置编译选项
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -D DEBUG") # 设置编译选项

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}) # 设置源文件目录
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build) # 设置编译目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # 设置可执行文件目录
set(CMAKE_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/utils) # 设置头文件目录
file(GLOB_RECURSE DIR_SRCS ${PROJECT_SOURCE_DIR}/src/main.c ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/utils/*.c) # 设置源文件目录

add_executable(dev_syn ${DIR_SRCS}) # 指定生成目标
target_include_directories(dev_syn PUBLIC ${CMAKE_INCLUDE_PATH}) # 指定头文件目录
