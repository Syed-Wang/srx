
/* csdn: 专题讲解
 * https://blog.csdn.net/ldl617/category_11380464.html
 */

#include <asm/types.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct plane_start {
    void* start;
};

struct buffer {
    struct plane_start* plane_start;
    struct v4l2_plane* planes_buffer;
};

/* // 查询设备是否支持多平面捕获
static int query_cap(int fd)
{
        struct v4l2_capability cap;
        int ret = 0;

        if ((ret = ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1){
        perror("VIDIOC_QUERYCAP");
        return ret;
    } */

int main(int argc, char** argv)
{
    int fd;
    fd_set fds;
    FILE* file_fd;
    struct timeval tv;
    int ret = -1, i, j, r;
    int num_planes;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane* planes_buffer;
    struct plane_start* plane_start;
    struct buffer* buffers;
    enum v4l2_buf_type type;

    if (argc != 4) {
        printf("Usage: v4l2_test <device> <frame_num> <save_file>\n");
        printf("example: v4l2_test /dev/video0 10 test.yuv\n");
        return ret;
    }

    // 打开设备
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("open device: %s fail\n", argv[1]);
        goto err;
    }

    // 打开保存文件
    file_fd = fopen(argv[3], "wb+");
    if (!file_fd) {
        printf("open save_file: %s fail\n", argv[3]);
        goto err1;
    }

    char* FILE_VIDEO = argv[3];

    // 获取设备信息
    // 查看设备支持的功能
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        printf("Error opening device %s: unable to query device.\n", FILE_VIDEO);
        return 0;
    } else {
        printf("driver:\t\t%s\n", cap.driver);
        printf("card:\t\t%s\n", cap.card);
        printf("bus_info:\t%s\n", cap.bus_info);
        printf("version:\t%d\n", cap.version);
        printf("capabilities:\t%x\n", cap.capabilities);

        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE) {
            printf("Device %s: supports capture.\n", FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) == V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
            printf("Device %s: supports mplane capture.\n", FILE_VIDEO);
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) {
            printf("Device %s: supports streaming.\n", FILE_VIDEO);
        }
    }

    printf("Support capture!\n");

    int IMAGEWIDTH = 1920;
    int IMAGEHEIGHT = 1080;

    struct v4l2_fmtdesc fmtdesc;
    // 显示所有支持帧格式
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    printf("Support format:\n");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1) {
        printf("\t%d.%s\n", fmtdesc.index + 1, fmtdesc.description);
        fmtdesc.index++;
    }
    
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix_mp.width = IMAGEWIDTH;
	fmt.fmt.pix_mp.height = IMAGEHEIGHT;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 1;
    if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        printf("Unable to set format\n");
        // return 0;
    }

    printf("get fmt...\n"); 
    if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
        printf("Unable to get format\n");
		perror("VIDIOC_G_FMT");
        // return 0;
    }
	else {
        printf("fmt.type:\t\t%d\n",fmt.type);
        printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix_mp.pixelformat & 0xFF, (fmt.fmt.pix_mp.pixelformat >> 8) & 0xFF,(fmt.fmt.pix_mp.pixelformat >> 16) & 0xFF, (fmt.fmt.pix_mp.pixelformat >> 24) & 0xFF);
        printf("pix.height:\t\t%d\n",fmt.fmt.pix_mp.height);
        printf("pix.width:\t\t%d\n",fmt.fmt.pix_mp.width);
        printf("pix.field:\t\t%d\n",fmt.fmt.pix_mp.field);
    }

// v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat='NV21' --stream-mmap=4 --stream-skip=0 --stream-to=/data/output_1080p_yuv420.yuv --stream-count=5 --stream-poll


    /*     struct v4l2_format fmt_tmp;

        if (ioctl(fd, VIDIOC_G_FMT, &fmt_tmp) < 0) {
            printf("Get format fail\n");
            goto err1;
        }
            printf("width = %d\n", fmt_tmp.fmt.pix_mp.width);
        printf("height = %d\n", fmt_tmp.fmt.pix_mp.height);
        printf("nmplane = %d\n", fmt_tmp.fmt.pix_mp.num_planes);
        printf("nmplane = %d\n", fmt_tmp.fmt.pix_mp.pixelformat);


        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.width = 1920; // 2400;
        fmt.fmt.pix_mp.height = 1080; // 1920;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_RGB24; // V4L2_PIX_FMT_SRGGB12;
        fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            printf("Set format fail\n");
            goto err1;
        } */

    /*     printf("width = %d\n", fmt.fmt.pix_mp.width);
        printf("height = %d\n", fmt.fmt.pix_mp.height);
        printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes); */

    // memset(&fmt, 0, sizeof(struct v4l2_format));
    // fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    // if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
    //	printf("Set format fail\n");
    //	goto err;
    // }

    // printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);

    req.count = 5;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        printf("Reqbufs fail\n");
        goto err1;
    }

    printf("buffer number: %d\n", req.count);

    num_planes = fmt.fmt.pix_mp.num_planes;

    buffers = malloc(req.count * sizeof(*buffers));

    printf("by dss   test VIDIOC_QUERYBUF\n");
    for (i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        planes_buffer = calloc(num_planes, sizeof(*planes_buffer));
        plane_start = calloc(num_planes, sizeof(*plane_start));
        memset(planes_buffer, 0, sizeof(*planes_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes_buffer;
        buf.length = num_planes;
        buf.index = i;
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            printf("Querybuf fail\n");
            req.count = i;
            // goto err2;
        }

        (buffers + i)->planes_buffer = planes_buffer;
        (buffers + i)->plane_start = plane_start;
        for (j = 0; j < num_planes; j++) {
            printf("plane[%d]: length = %d\n", j, (planes_buffer + j)->length);
            printf("plane[%d]: offset = %d\n", j, (planes_buffer + j)->m.mem_offset);
            (plane_start + j)->start = mmap(NULL /* start anywhere */,
                (planes_buffer + j)->length,
                PROT_READ | PROT_WRITE /* required */,
                MAP_SHARED /* recommended */,
                fd,
                (planes_buffer + j)->m.mem_offset);
            if (MAP_FAILED == (plane_start + j)->start) {
                printf("mmap failed\n");
                req.count = i;
                // goto unmmap;
            }
        }
    }
    printf("by dss test VIDIOC_QBUF\n");
    for (i = 0; i < req.count; ++i) {
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = num_planes;
        buf.index = i;
        buf.m.planes = (buffers + i)->planes_buffer;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            printf("VIDIOC_QBUF failed\n");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        printf("VIDIOC_STREAMON failed\n");

    int num = 0;
    struct v4l2_plane* tmp_plane;
    tmp_plane = calloc(num_planes, sizeof(*tmp_plane));
    printf("by dss  test  while (1)\n");
    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        printf("by dss  test select  \n");
        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno) {
                printf("by dss  test EINTR == errno\n");
                continue;
            }
            printf("select err\n");
        }
        if (0 == r) {

            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = tmp_plane;
        buf.length = num_planes;
        printf("by dss  test VIDIOC_DQBUF  \n");
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
            printf("dqbuf fail\n");

        printf("by dss  test  fwrite\n");
        for (j = 0; j < num_planes; j++) {
            printf("plane[%d] start = %p, bytesused = %d\n", j, ((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused);
            fwrite(((buffers + buf.index)->plane_start + j)->start, (tmp_plane + j)->bytesused, 1, file_fd);
        }

        num++;
        if (num >= atoi(argv[2])) {
            printf("by dss  test  fwrite num %d\n", num);
            break;
        }
        printf("by dss  test  VIDIOC_QBUF\n");
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            printf("failture VIDIOC_QBUF\n");
    }

    // id

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        printf("VIDIOC_STREAMOFF fail\n");

    free(tmp_plane);

    ret = 0;
unmmap:
err2:
    for (i = 0; i < req.count; i++) {
        for (j = 0; j < num_planes; j++) {
            if (MAP_FAILED != ((buffers + i)->plane_start + j)->start) {
                if (-1 == munmap(((buffers + i)->plane_start + j)->start, ((buffers + i)->planes_buffer + j)->length))
                    printf("munmap error\n");
            }
        }
    }

    for (i = 0; i < req.count; i++) {
        free((buffers + i)->planes_buffer);
        free((buffers + i)->plane_start);
    }

    free(buffers);

    fclose(file_fd);
err1:
    close(fd);
err:
    return ret;
}
