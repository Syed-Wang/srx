#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__

#define SYS_CONFIG_PATH "../config/sys_config.json" // JSON 文件路径

extern char ip[128][16]; // ip 数组
extern int node_num; // 组网中节点数量

// 帧率结构体
typedef struct {
    unsigned char frame_rate; // 帧率 (默认 60)
    unsigned char sync_encoder; // 同步编码器 (预留) (0 不同步 1 同步)
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主) (0指定服务器 1自主)
    char (*ip)[16]; // IP 地址表
} fps_t;

// 窗口结构体
typedef struct {
    unsigned char id; // 设备 id (从 1 开始)
    char src_ip[16]; // 数据源 IP
    int x; // 窗口 x 坐标
    int y; // 窗口 y 坐标
    int w; // 窗口宽度
    int h; // 窗口高度
} window_t;

extern fps_t fps; // 帧率结构体
extern window_t window; // 窗口结构体

/**
 * @brief 场景保存：帧率和布局保存为 JSON 文件
 *
 * @param fps 接收帧率结构体
 * @param window 接收布局结构体
 * @return int 0 成功 -1 失败
 */
int save_sys_config(fps_t* fps, window_t* window);

/**
 * @brief 场景加载：帧率和布局从 JSON 文件中加载
 *
 * @param fps 接收帧率结构体
 * @param window 接收布局结构体
 * @return int 0 成功 -1 失败
 */
int load_sys_config(fps_t* fps, window_t* window);

/**
 * @brief 设置组网标志
 *
 * @param fps 帧率结构体
 * @param flag 组网标志
 * @return int 成功返回 0，失败返回 -1
 */
int set_net_flag(fps_t* fps, unsigned char flag);

#endif // __SYS_CONFIG_H__