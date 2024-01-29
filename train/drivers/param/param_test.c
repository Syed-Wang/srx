#include <linux/init.h>
#include <linux/module.h>

static int param_int = 10;
static char* param_str = "hello";

static int __init param_init(void)
{
    printk(KERN_INFO "param_init\n");
    printk(KERN_INFO "param_int = %d\n", param_int);
    printk(KERN_INFO "param_str = %s\n", param_str);

    return 0;
}

static void __exit param_exit(void)
{
    printk(KERN_INFO "param_exit\n");
	printk(KERN_INFO "param_int = %d\n", param_int);
	printk(KERN_INFO "param_str = %s\n", param_str);
}

module_param(param_int, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH == 0644
module_param(param_str, charp, S_IRUGO); // S_IRUGO == 0444
module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");