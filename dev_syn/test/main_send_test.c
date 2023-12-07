#include "h264.h"
#include "tool.h"
#include <stdio.h>

int main(int argc, const char* argv[])
{
    // BGR3(bgr24): 65543
    // ./dev_syn -w 1920 -h 1080 -t 7 -f 65543 -i /dev/video0
    if (send_h264(argc, (char**)argv) < 0) {
        PTRERR("send_h264 error");
        return -1;
    }
    printf("send_h264 success\n");

    return 0;
}