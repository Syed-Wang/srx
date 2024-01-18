#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define GPIO_OUT_PIN 42 // 输出引脚
#define GPIO_IN_PIN 41 // 输入引脚

static unsigned int irqNumber;

static irq_handler_t gpio_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs)
{
    // printk(KERN_INFO "GPIO interrupt!\n");
    return (irq_handler_t)IRQ_HANDLED; // 返回 IRQ_HANDLED 表示中断已处理
}

static int __init gpio_init(void)
{
    int ret = 0;

    if (!gpio_is_valid(GPIO_OUT_PIN)) { // 检查引脚是否有效
        printk(KERN_INFO "GPIO_TEST: output pin is not valid.\n");
        return -ENODEV;
    }

    gpio_request(GPIO_OUT_PIN, "sysfs");
    gpio_direction_output(GPIO_OUT_PIN, 0);
    gpio_export(GPIO_OUT_PIN, false);

    if (!gpio_is_valid(GPIO_IN_PIN)) {
        printk(KERN_INFO "GPIO_TEST: input pin is not valid.\n");
        return -ENODEV;
    }

    gpio_request(GPIO_IN_PIN, "sysfs");
    gpio_direction_input(GPIO_IN_PIN);
    gpio_set_debounce(GPIO_IN_PIN, 200);
    gpio_export(GPIO_IN_PIN, false);

    irqNumber = gpio_to_irq(GPIO_IN_PIN);
    ret = request_irq(irqNumber, (irq_handler_t)gpio_irq_handler, IRQF_TRIGGER_RISING, "gpio_test_irq", NULL);

    if (ret) {
        printk(KERN_INFO "GPIO_TEST: cannot register IRQ %d\n", irqNumber);
        return -EIO;
    }

    printk(KERN_INFO "GPIO_TEST: initialized.\n");

    return ret;
}

static void __exit gpio_exit(void)
{
    gpio_set_value(GPIO_OUT_PIN, 0);
    gpio_unexport(GPIO_OUT_PIN);
    free_irq(irqNumber, NULL);
    gpio_unexport(GPIO_IN_PIN);
    gpio_free(GPIO_OUT_PIN);
    gpio_free(GPIO_IN_PIN);

    printk(KERN_INFO "GPIO_TEST: exited.\n");
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");