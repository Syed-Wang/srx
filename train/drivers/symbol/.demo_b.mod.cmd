cmd_/home/srx/work/train/drivers/symbol/demo_b.mod := printf '%s\n'   demo_b.o | awk '!x[$$0]++ { print("/home/srx/work/train/drivers/symbol/"$$0) }' > /home/srx/work/train/drivers/symbol/demo_b.mod
