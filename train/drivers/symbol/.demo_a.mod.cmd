cmd_/home/srx/work/train/drivers/symbol/demo_a.mod := printf '%s\n'   demo_a.o | awk '!x[$$0]++ { print("/home/srx/work/train/drivers/symbol/"$$0) }' > /home/srx/work/train/drivers/symbol/demo_a.mod
