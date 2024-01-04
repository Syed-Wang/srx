#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h> // module_init, module_exit
#include <linux/module.h> // MODULE_LICENSE
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE 0x1000 // 4KB
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230

static int globalmem_major = GLOBALMEM_MAJOR; // 主设备号
module_param(globalmem_major, int, S_IRUGO); // 模块参数

struct globalmem_dev { // 设备结构体
    struct cdev cdev;
    unsigned char mem[GLOBALMEM_SIZE];
};

struct globalmem_dev* globalmem_devp;

static int __init globalmem_init(void){
	int ret;
	dev_t devno = MKDEV(globalmem_major, 0);
}
