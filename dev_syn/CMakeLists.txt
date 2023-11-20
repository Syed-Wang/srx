# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.15)
# 项目信息
project(dev_syn)
# 版本号
set(VERSION_MAJOR 1) # 主版本号
set(VERSION_MINOR 0) # 次版本号
set(VERSION_PATCH 0) # 补丁版本号

set(CMAKE_C_COMPILER "aarch64-none-linux-gnu-gcc") # 指定编译器 ARM
# set(CMAKE_C_COMPILER "gcc") # 指定编译器 X86
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -fPIE -Wno-psabi -w") # 设置编译选项 -Wno-psabi:关闭GCC对ABI的警告

set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build) # 设置编译目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) # 设置可执行文件目录

set(INCLUDE_PATH /home/srx/rk3588/external/mpp) # 指定文件路径
set(MYSYSROOT /home/srx/rk3588/buildroot/output/rockchip_rk3588/host/aarch64-buildroot-linux-gnu/sysroot)
set(CMAKE_SYSROOT ${MYSYSROOT}) # 设置库文件路径(交叉编译时需要设置)

file(GLOB_RECURSE DIR_SRCS
	${PROJECT_SOURCE_DIR}/src/*.c
	${PROJECT_SOURCE_DIR}/utils/*.c
	${INCLUDE_PATH}/utils/*.c
) # 设置源文件目录

add_executable(dev_syn ${DIR_SRCS}) # 指定生成目标

target_compile_definitions(dev_syn PUBLIC
	_GNU_SOURCE # 使用GNU扩展(struct ip_mreqn)
	ARM
	# X86
	# DEBUG # 打印调试信息
) # 设置宏定义(-D)
target_link_libraries(dev_syn PUBLIC
	rockchip_mpp
	pthread
) # 设置链接库(-l)
target_include_directories(dev_syn PUBLIC 
	${PROJECT_SOURCE_DIR}/inc
	${PROJECT_SOURCE_DIR}/utils
	${INCLUDE_PATH}/inc
	${INCLUDE_PATH}/utils
	${INCLUDE_PATH}/osal/inc
	${INCLUDE_PATH}/mpp/base/inc
) # 设置头文件目录(-I)
