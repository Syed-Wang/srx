#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* const argv[])
{
    if (argc != 7) {
        printf("Usage: %s -i <gpio_in_pin> -o <gpio_out_pin> -s <gpio_out_sequence>\n", argv[0]);
        return -1;
    }

    int gpio_in_pin = 0;
    int gpio_out_pin = 0;
    char gpio_out_sequence[128] = { 0 }; // 最多支持 128 个输出

    int opt;
    while ((opt = getopt(argc, argv, "i:o:s:")) != -1) {
        switch (opt) {
        case 'i':
            gpio_in_pin = atoi(optarg); // 41
            break;
        case 'o':
            gpio_out_pin = atoi(optarg); // 42
            break;
        case 's':
            strncpy(gpio_out_sequence, optarg, sizeof(gpio_out_sequence));
            break;
        default:
            printf("Usage: %s -i <gpio_in_pin> -o <gpio_out_pin> -s <gpio_out_sequence>\n", argv[0]);
            return -1;
        }
    }

    // 检查参数
    if (gpio_in_pin == gpio_out_pin) {
        printf("Error: gpio_in_pin == gpio_out_pin\n");
        return -1;
    }
    int sequence_size = strlen(gpio_out_sequence);
    // 检查序列是否只包含 0 和 1
    for (int i = 0; i < sequence_size; i++) {
        if (gpio_out_sequence[i] != '0' && gpio_out_sequence[i] != '1') {
            printf("Error: gpio_out_sequence should only contain 0 and 1\n");
            return -1;
        }
    }

    // 打开 gpio 输出文件
    char gpio_out_file[64] = { 0 };
    sprintf(gpio_out_file, "/sys/class/gpio/gpio%d/value", gpio_out_pin);
    int gpio_out_fd = open(gpio_out_file, O_WRONLY);
    if (gpio_out_fd < 0) {
        printf("Error: open %s failed\n", gpio_out_file);
        perror("");
        return -1;
    }

    // 打开 gpio 输入文件
    char gpio_in_file[64] = { 0 };
    sprintf(gpio_in_file, "/sys/class/gpio/gpio%d/value", gpio_in_pin);
    int gpio_in_fd = open(gpio_in_file, O_RDONLY);
    if (gpio_in_fd < 0) {
        printf("Error: open %s failed", gpio_in_file);
        perror("");
        close(gpio_out_fd);
        return -1;
    }

    char gpio_in_value = 0;
    printf("gpio_in_value: ");
    // 边写边读
    for (int i = 0; i < sequence_size; i++) {
        // 重置文件指针
        // lseek(gpio_out_fd, 0, SEEK_SET); // 系统自动重置
        lseek(gpio_in_fd, 0, SEEK_SET);

        // 写
        if (write(gpio_out_fd, &gpio_out_sequence[i], 1) < 0) {
            perror("Error: write failed");
            break;
        }
        // 读
        if (read(gpio_in_fd, &gpio_in_value, 1) < 0) {
            perror("Error: read failed");
            break;
        }
        printf("%c", gpio_in_value);
    }
    printf("\n");

    // 关闭文件
    close(gpio_out_fd);
    close(gpio_in_fd);

    return 0;
}