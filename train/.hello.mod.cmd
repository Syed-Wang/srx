cmd_/home/srx/work/train/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/srx/work/train/"$$0) }' > /home/srx/work/train/hello.mod
