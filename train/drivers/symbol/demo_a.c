#include <linux/init.h>
#include <linux/module.h>

int add_integar(int a, int b)
{
	return a + b;
}
EXPORT_SYMBOL_GPL(add_integar);

static int __init demo_init(void)
{
	printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);

	return 0;
}

static void __exit demo_exit(void)
{
	printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");

