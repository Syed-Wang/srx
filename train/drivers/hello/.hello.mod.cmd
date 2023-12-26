cmd_/home/srx/work/train/drivers/hello/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/srx/work/train/drivers/hello/"$$0) }' > /home/srx/work/train/drivers/hello/hello.mod
