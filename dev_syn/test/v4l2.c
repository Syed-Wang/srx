#include <assert.h>
#include <errno.h>
#include <fcntl.h> /* low-level i/o */
#include <getopt.h> /* getopt_long() */
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x)) // 清空结构体

struct buffer { // 缓冲区结构体
    void* start; // 缓冲区起始地址
    size_t length; // 缓冲区长度
};

static char* dev_name; // 设备名
static int fd = -1; // 文件描述符
struct buffer* buffers; // 缓冲区数组
static unsigned int n_buffers; // 缓冲区个数
static int frame_count = 70; // 采集帧数
FILE* fp; // 文件指针

/**
 * @brief 错误处理函数
 *
 * @param s 错误信息
 */
static void errno_exit(const char* s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

/**
 * @brief ioctl函数
 *
 * @param fh 文件描述符
 * @param request ioctl命令
 * @param arg 参数
 * @return int
 */
static int xioctl(int fh, int request, void* arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

/**
 * @brief 处理图像
 *
 * @param p 图像数据
 * @param size 图像大小
 */
static void process_image(const void* p, int size)
{
    fwrite(p, size, 1, fp);
}

/**
 * @brief 读取一帧
 *
 * @param fp 文件指针
 * @return int 1-成功，0-失败
 */
static int read_frame(FILE* fp)
{
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
        errno_exit("VIDIOC_DQBUF");

    process_image(buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");

    return 1;
}

/**
 * @brief 主循环
 *
 * @note 读取一帧，处理一帧
 */
static void mainloop(void)
{
    unsigned int count;
    count = frame_count;
    while (count-- > 0) {
        printf("No.%d\n", frame_count - count); // 显示当前帧数目
        read_frame(fp);
    }
    printf("\nREAD AND SAVE DONE!\n");
}

/**
 * @brief 停止采集
 *
 */
static void stop_capturing(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
        errno_exit("VIDIOC_STREAMOFF");
}

/**
 * @brief 开始采集
 *
 */
static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type; // 帧类型

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) // 将缓冲帧放入队列
            errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) // 开始采集
        errno_exit("VIDIOC_STREAMON");
}

/**
 * @brief 释放内存
 *
 */
static void uninit_device(void)
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    free(buffers);
}

/**
 * @brief 初始化内存
 *
 */
static void init_mmap(void)
{
    struct v4l2_requestbuffers req; // 请求缓冲区结构体

    CLEAR(req);

    req.count = 4; // 缓冲区个数
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 帧类型
    req.memory = V4L2_MEMORY_MMAP; // 内存类型

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) { // 请求分配缓冲区
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n",
                dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n",
            dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

/**
 * @brief 初始化设备
 *
 */
static void init_device(void)
{
    struct v4l2_capability cap; // 设备属性
    struct v4l2_format fmt; // 帧格式

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) { // 获取设备属性
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) { // 判断是否支持视频采集
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) { // 判断是否支持流式i/o
        fprintf(stderr, "%s does not support streaming i/o\n",
            dev_name);
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt); // 清空结构体
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 设置帧类型
    fmt.fmt.pix.width = 640; // 设置帧宽
    fmt.fmt.pix.height = 480; // 设置帧高
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // 设置帧格式
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; // 设置扫描方式

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) // 设置帧格式
        errno_exit("VIDIOC_S_FMT");

    init_mmap();
}

/**
 * @brief 关闭设备
 *
 */
static void close_device(void)
{
    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}

/**
 * @brief 打开设备
 *
 */
static void open_device(void)
{
    // 加上O_NONBLOCK会出现如下错误
    // VIDIOC_DQBUF error 11, Resource temporarily unavailable
    fd = open(dev_name, O_RDWR /* required */ /*| O_NONBLOCK*/, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        exit(1);
    }
}

int main(int argc, char** argv)
{
    dev_name = "/dev/video0"; // 默认设备名

    if (argc != 2) {
        printf("usage: %s filename\n", argv[0]);
        exit(0);
    }
    if ((fp = fopen(argv[1], "w")) == NULL) {
        perror("Creat file failed");
        exit(0);
    }
    open_device(); // 打开设备
    init_device(); // 设置视频格式
    start_capturing(); // 请求分配缓冲，获取内核空间，
    mainloop(); // 主要处理均在该函数中实现

    fclose(fp);
    stop_capturing();
    uninit_device();
    close_device();
    // fprintf(stderr, "\n");
    return 0;
}