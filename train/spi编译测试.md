# spi 编译测试

## 1. 内核配置

```bash
make ARCH=arm64 menuconfig
```

```text
Device Drivers --->
    [*] SPI support --->
        [*] User mode SPI device driver support
```

## 2. DTS 配置

修改 `kernel/arch/arm64/boot/dts/rockchip/rk3588-evb7-lp4-v10-linux.dts` 文件, 添加如下内容:

```c
&spi0 {
    status = "okay";
    max-freq = <50000000>;
    pinctrl-0 = <&spi0m2_cs0 &spi0m2_cs1 &spi0m2_pins>;
    spi_test@0 {
        compatible = "rockchip,spidev";
        reg = <0>;
        spi-max-frequency = <5000000>;
    };
};
```

## 3. 编译

在 SDK 目录下执行:

```bash
sudo ./build.sh
```

（使用 sudo ./build.sh update 生成的 update.img 没有 /dev/spidevB.C）

生成的 update.img 在 `rockdev` 目录下，然后烧录到板子上。

## 4. 测试

编译测试程序 `kernel/tools/spi/spidev_test.c` :

```bash
make spidev_test CC=aarch64-none-linux-gnu-gcc LD=aarch64-none-linux-gnu-ld
```

将编译好的程序拷贝到板子上，测试：

```bash
./spidev_test -HOv -D /dev/spidev0.0 -p "1234\xde\xad"
```

![测试结果](https://s2.loli.net/2023/12/27/eix1ELyu2DfUSdz.png)

选项:

```text
-D --device   device to use (default /dev/spidev1.1)
-s --speed    max speed (Hz)
-d --delay    delay (usec)
-b --bpw      bits per word
-i --input    input data from a file (e.g. \"test.bin\")
-o --output   output data to a file (e.g. \"results.bin\")
-l --loop     loopback
-H --cpha     clock phase
-O --cpol     clock polarity
-L --lsb      least significant bit first
-C --cs-high  chip select active high
-3 --3wire    SI/SO signals shared
-v --verbose  Verbose (show tx buffer)
-p            Send data (e.g. \"1234\\xde\\xad\")
-N --no-cs    no chip select
-R --ready    slave pulls low to pause
-2 --dual     dual transfer
-4 --quad     quad transfer
-8 --octal    octal transfer
-S --size     transfer size
-I --iter     iterations
```
