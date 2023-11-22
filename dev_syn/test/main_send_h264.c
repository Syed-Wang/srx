#include "h264.h"
#include "tool.h"
#include <stdio.h>

int main(int argc, const char* argv[])
{
    // ./dev_syn -w 1280 -h 720 -f 30 -t 7 -i ./sample.yuv
    if (send_h264(argc, (char**)argv) < 0) {
        PTRERR("send_h264 error");
        return -1;
    }
    printf("send_h264 success\n");

    return 0;
}