# Architecture
arch ?= x86
# Module name
modname ?= demo

# Kernel modules
obj-m := $(modname).o

# 如果一个模块包含多个源文件，可以添加如下语句
# $(modname)-objs := demo1.o demo2.o

ifeq ($(arch), arm)
	KERNELDIR := /home/srx/rk3588/kernel

	# CROSS_COMPILE := aarch64-none-linux-gnu-
	# CC := $(CROSS_COMPILE)gcc
	# LD := $(CROSS_COMPILE)ld
else
	# Kernel version
	KVERS = $(shell uname -r)
	KERNELDIR := /lib/modules/$(KVERS)/build
endif

# Specify flags for the module compilation.
# EXTRA_CFLAGS = -g -O0
# -g(gdb): debug info, -O0: no optimization

# 使用外部符号表必须指定路径
# KBUILD_EXTRA_SYMBOLS+= /home/srx/work/train/drivers/symbol/Module.symvers

.PHONY: build clean

build: kernel_modules

kernel_modules:
	make -C $(KERNELDIR) M=$(CURDIR) modules
clean:
	make -C $(KERNELDIR) M=$(CURDIR) clean