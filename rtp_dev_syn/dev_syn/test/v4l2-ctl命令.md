# v4l2-ctl 命令

v4l2-ctl 命令用于控制 V4L2 设备，可以用来调整视频设备的亮度、对比度等参数，也可以用来调整摄像头的参数，如曝光、白平衡等。

1. 列出所有的视频设备：`v4l2-ctl --list-devices`
2. 查询特定视频设备的能力：`v4l2-ctl -d /dev/video0 --all`
3. 查询特定视频设备的格式：`v4l2-ctl -d /dev/video0 --list-formats-ext`

|                              命令                               |                         描述                          |
| :-------------------------------------------------------------: | :---------------------------------------------------: |
|                        `ls /dev/video*`                         |                查看总共多少 video 节点                |
|          `grep '' /sys/class/video4linux/video*/name`           |                找到 hdmirx 节点 videox                |
|             `v4l2-ctl --verbose -d /dev/video0 -D`              |               查看当前 video 节点的状态               |
|          `v4l2-ctl -d /dev/video0 --query-dv-timings`           | 实时查询分辨率、逐\隔行，像素时钟，HBP,HFP,HSYNC 等等 |
|           `v4l2-ctl -d /dev/video0 --get-dv-timings`            |               查询已存储的 timing 信息                |
|     `cat /sys/kernel/debug/clk/clk_summary \| grep hdmirx`      |                      获取时钟树                       |
|              `v4l2-ctl -d /dev/video0 --get-edid`               |                       读取 edid                       |
|               `vim rkbin/RKTRUST/RK3588TRUST.ini`               |                    查询 bl31 版本                     |
| `v4l2-ctl -d /dev/v4l-subdev2 --poll-for-event=source_change=0` |              获取 v4l2 上报 subdev 事件               |
|   `v4l2-ctl -d /dev/video0 --poll-for-event=source_change=0`    |               获取 v4l2 上报 video 事件               |
|              `cat /sys/kernel/debug/hdmirx/status`              |                 获取 hdmirx 当前状态                  |
|               `cat /sys/kernel/debug/hdmirx/ctrl`               |                 dump 控制器寄存器列表                 |
|              `cat /sys/kernel/debug/dri/0/summary`              |                     获取图层信息                      |
|          `adb shell setprop persist.sys.hdmiinmode 2`           |                打开 camera 预览 hdmiin                |
|             `echo 2 > sys/class/hdmirx/hdmirx/edid`             |                  切换 edid 分组 600M                  |
|             `echo 1 > sys/class/hdmirx/hdmirx/edid`             |                  切换 edid 分组 340M                  |
|            `cat /sys/class/misc/hdmirx_hdcp/status`             |                    获取 hdcp 状态                     |
|          `echo 2 >/sys/class/misc/hdmirx_hdcp/enable`           |                 设置 hdcp1.x、hdcp2.x                 |
|                    `io -4 0xfdee0430 0xff00`                    | 设置 RGB AVMUTE 红色，若是其他 yuv 格式可能会变色灰色 |
|          `cat /sys/class/hdmirx/hdmirx/audio_present`           |                  audio 设备拔插状态                   |
|            `cat /sys/class/hdmirx/hdmirx/audio_rate`            |                     audio 采样率                      |

获取数据流：3588-hdmirx 获取数据流需要关闭预览:  
`v4l2-ctl --verbose -d /dev/video0 --set-fmt-video=width=1920,height=1080 pixelformat='BGR3' --stream-mmap=4 --set-selection=target=crop,flags=0,top=0,left=0,width=1920,height=1080`  
抓取数据帧:  
`v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat='BGR3' --stream-mmap=4 --stream-skip=0 --stream-to=/data/output_1080p_BGR888.yuv --stream-count=5 --stream-poll`
