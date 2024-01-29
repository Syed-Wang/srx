#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
    printk(KERN_INFO "Hello World enter\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Hello World exit\n ");
}

module_init(hello_init); // 指定模块初始化函数
module_exit(hello_exit); // 指定模块退出函数

MODULE_LICENSE("GPL"); // 声明模块许可证
