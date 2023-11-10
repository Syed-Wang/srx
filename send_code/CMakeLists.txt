#CMake最低版本号要求
cmake_minimum_required(VERSION 2.8)

#指定项目名称
project(send_code)

#指定工程需要用到的一些库
#set(ENV{PKG_CONFIG_PATH} /usr/local/AID/pkgconfig)
#find_package(PkgConfig)
#pkg_search_module(BLADECV REQUIRED bladecv)
#pkg_search_module(GTK REQUIRED gtk±3.0)

#find_package(PkgConfig)
#find_package(Qt5 REQUIRED Widgets)
#find_package(Qt5 REQUIRED Core)
#find_package(Qt5 REQUIRED Gui)

#sysroot被称为逻辑根目录，只在链接过程中起作用，作为交叉编译工具链搜索库文件的根路径，如配置--sysroot=dir，则dir作为逻辑根目录，链接器将在dir/usr/lib中搜索库文件。
# libdrm in the sysroot path
set(MYSYSROOT /home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot)



# 指定版本信息
set(CMAKE_SYSTEM_VERSION 1)

# 若是需要指定编译器路径
set(CROSS_TOOLCHAIN_PREFIX "/home/linux/rk356x_linux_230210/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-")
# 指定编译器
#set(CMAKE_C_COMPILER   “KaTeX parse error: Expected 'EOF', got '#' at position 31: …N_PREFIX}gcc") #?set(CMAKE_CXX_C…{CROSS_TOOLCHAIN_PREFIX}g++”)
set(CMAKE_C_COMPILER ${CROSS_TOOLCHAIN_PREFIX}gcc) # 使用默认路径的gcc指定编译器
#set(CMAKE_C_COMPILER ${CROSS_TOOLCHAIN_PREFIX}g++)
#使用默认路径的g++指定编译器  
set(CMAKE_CXX_COMPILER ${CROSS_TOOLCHAIN_PREFIX}g++) # 使用默认路径的g++指定编译器

# 指定
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ldrm -lrockchip_mpp -lpthread")
# C 编译器flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie -fPIE -lc -ldl -lm ")
#出现error的原因是C++设计的比C更加安全，它不能自动的将void *转换为其它指针类型,解决方法：CFLAGS = -fpermissive
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g ")
#包含c++11特性
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -g   -fpermissive  ")

# compiler/linker flags
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)

# set(CMAKE_FIND_ROOT_PATH "${MYSYSROOT}")
# SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)


# 指定编译选项
set(CMAKE_BUILD_TYPE Debug )

# 指定源码目录
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 指定编译目录
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

#添加子目录,这样进入源码文件src目录可以继续构建  
#add_subdirectory(${PROJECT_SOURCE_DIR}/src)
#add_subdirectory(${PROJECT_SOURCE_DIR}/utils) #need Cmakelist.txt
#查找当前目录下的所有源文件，并将名称保存到DIR_LIB_SRCS目录
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/ DIR_LIB_SRCS)
file(GLOB_RECURSE DIR_LIB_SRCS ${PROJECT_SOURCE_DIR}/src/main.c ${PROJECT_SOURCE_DIR}/src/**/*.c ${PROJECT_SOURCE_DIR}/utils/*.c)

# 指定头文件路径
set(INCLUDE_PATH /home/linux/rk356x_linux_230210/external/mpp)

#指定头文件目录,PROJECT_SOURCE_DIR为工程的根目录  
include_directories(
${PROJECT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}/include
/usr/local/AID
${BLADECV_INCLUDE_DIRS}
${GTK_INCLUDE_DIRS}
#drm header
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/libdrm-2.4.109/include/drm
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/libdrm-2.4.109
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/rockchip
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/rockchip-mpp-develop/osal/inc
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/rockchip-mpp-develop/utils
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/rockchip-mpp-develop/inc
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/rockchip-mpp-develop/inc
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/build/rockchip-mpp-develop/mpp/base/inc

/home/linux/srx/send_code/inc/cmd_inc
/home/linux/srx/send_code/inc/h264_inc
/home/linux/srx/send_code/inc/time_inc
${INCLUDE_PATH}/inc
${INCLUDE_PATH}/utils
${INCLUDE_PATH}/osal/inc
${INCLUDE_PATH}/mpp/inc
${PROJECT_SOURCE_DIR}/utils
)

# 添加需要链接的库文件目录
link_directories(
/usr/lib64/
${BLADECV_LIBRARY_DIRS}
${GTK_LIBRARY_DIRS}
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib
/home/linux/rk356x_linux_230210/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/lib64
)

#添加编译参数和横定义
#option(USE_BLADECV “bladecv” ON)
#if(USE_BLADECV)
#add_definitions(-DBLADECV)
#endif()

#设置编译选项(会将gcc g++全部加上)
#add_compile_options(-g -std=c++11)
add_compile_options(-fPIC)

#指定可执行文件的输出目录，输出到bin下面  
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#生成动态库    
#add_library(shared_demo SHARED demo.cpp -lrtspclient -lrockchip_rga -lrockchip_mpp -ldrm -lpthread
#-lbcv -lgtk-3 -lrtspclient)

#设置库输出名为 shared => libshared.so  
#set_target_properties(shared_demo PROPERTIES OUTPUT_NAME “shared”)

#生成静态库  
#add_library(static_demo STATIC demo.cpp -lrtspclient -lrockchip_rga -lrockchip_mpp -ldrm -lpthread
#-lbcv -lgtk-3 -lrtspclient)  
#设置输库出名为 static => libstatic.a  
#set_target_properties(static_demo PROPERTIES OUTPUT_NAME “static”)

#指定库文件输出路径  
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#在指定目录下查找库，并保存在LIBPATH变量中
#find_library(LIBPATHS  shared ${PROJECT_SOURCE_DIR}/lib /usr/lib64)

# 指定生成目标
add_executable(send_code ${DIR_LIB_SRCS}
# ffmpeg_drm.c
# modeset-vsync.c
# mpi_dec_dss_test.c 
# drm_flip_plane_test.c
# ./utils/mpi_dec_utils.c
# ./utils/utils.c
# ./utils/mpp_opt.c
# xxxx.c
# xxxxx.c
# ./src/main.c
# ./src/cmd_src/cmd_send.c
# ./src/h264_src/h264_send.c
# ./src/time_src/time_send.c
)


#链接共享库
#target_link_libraries(demo -lrtspclient -lrockchip_rga -lrockchip_mpp -ldrm -lpthread
#-lbcv -lgtk-3 -lrtspclient ${BLADECV_LDFLAGS} ${GTK_LDFLAGS})
