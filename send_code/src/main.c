#include "cmd_send.h"
#include "h264_send.h"
#include "time_send.h"

int main(int argc, char** argv)
{
    if (send_time() < 0) {
        printf("send_time error\n");
        return -1;
    }

    if (send_cmd() < 0) {
        printf("send_cmd error\n");
        return -1;
    }

    if (send_h264(argc, argv) < 0) {
        printf("send_h264 error\n");
        return -1;
    }

    return 0;
}