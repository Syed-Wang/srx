# gpio 测试

1. 通过`export`导出gpio口:（驱动代码自动导出）
   - `echo 4 > /sys/class/gpio/export`
   - `echo in > /sys/class/gpio/gpio4/direction`（in/out）
2. 读取gpio口的值:
   - `cat /sys/class/gpio/gpio4/value`
3. 设置gpio口的值:
   - `echo 1 > /sys/class/gpio/gpio4/value`（1/0）
4. 取消gpio口的导出:（驱动代码自动取消）
   - `echo 4 > /sys/class/gpio/unexport`
