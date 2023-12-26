arch?=x86
modname?=demo
ifeq ($(arch),arm)
	KERNELDIR:=/home/srx/rk3588/kernel
else
	KERNELDIR:=/lib/modules/$(shell uname -r)/build
endif
PWD:= $(shell pwd) #当前模块路径

all:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean

obj-m:=$(modname).o

# make arch=arm modname=demo