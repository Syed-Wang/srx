cmd_/home/srx/work/train/drivers/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/srx/work/train/drivers/"$$0) }' > /home/srx/work/train/drivers/hello.mod
