# 编译 send_code 程序
# 将源文件编译成静态库，然后链接到可执行文件中
# 生成的可执行文件在 bin 目录下

CC = gcc # 编译器
INCLUDE = -I./inc/** # 头文件路径
LIBS = -L./lib/** # 静态库路径
LIBS += -l** # 静态库名
SRC = $(wildcard ./src/*.c) # 源文件
OBJ = $(patsubst %.c, %.o, $(SRC)) # 目标文件
BIN = ./bin/send_code # 可执行文件

$(BIN): $(OBJ)
	$(CC) $(INCLUDE) $(LIBS) -o $@ $^
%.o: %.c
	$(CC) $(INCLUDE) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ) $(BIN)