#include "sys_config.h"
#include "cJSON.h"
#include <stdio.h>

int save_sys_config(fps_t* fps, window_t* window)
{
    FILE* fp = fopen(SYS_CONFIG_PATH, "w"); // w 清空再写入
    if (fp == NULL) {
        perror("fopen sys_config error");
        return -1;
    }

    // 将 fps 和 window 结构体写入 JSON 文件
    cJSON* root = cJSON_CreateObject(); // 创建 JSON 对象
    cJSON* fps_obj = cJSON_CreateObject(); // 创建 fps 对象
    cJSON* window_obj = cJSON_CreateObject(); // 创建 window 对象

    // fps 对象
    cJSON_AddNumberToObject(fps_obj, "frame_rate", fps->frame_rate);
    cJSON_AddNumberToObject(fps_obj, "sync_encoder", fps->sync_encoder);
    cJSON_AddNumberToObject(fps_obj, "priority", fps->priority);
    cJSON_AddNumberToObject(fps_obj, "net_flag", fps->net_flag);

    // window 对象
    cJSON_AddNumberToObject(window_obj, "id", window->id);
    cJSON_AddStringToObject(window_obj, "src_ip", window->src_ip);
    cJSON_AddNumberToObject(window_obj, "x", window->x);
    cJSON_AddNumberToObject(window_obj, "y", window->y);
    cJSON_AddNumberToObject(window_obj, "w", window->w);
    cJSON_AddNumberToObject(window_obj, "h", window->h);

    // root 对象
    cJSON_AddItemToObject(root, "fps", fps_obj); // 添加 fps 对象到 root 对象
    cJSON_AddItemToObject(root, "window", window_obj); // 添加 window 对象到 root 对象

    // 将 JSON 对象转换为字符串
    char* json_str = cJSON_Print(root);
    if (json_str == NULL) {
        perror("cJSON_Print error");
        return -1;
    }

    // 写入 JSON 字符串到文件
    if (fwrite(json_str, strlen(json_str), 1, fp) != 1) {
        perror("fwrite sys_config error");
        return -1;
    }

    // 释放内存
    free(json_str);
    cJSON_Delete(root);
    fclose(fp);

    return 0;
}

int load_sys_config(fps_t* fps, window_t* window)
{
    FILE* fp = fopen(SYS_CONFIG_PATH, "r"); // r 只读
    if (fp == NULL) {
        perror("fopen sys_config error");
        return -1;
    }

    // 读取 JSON 文件内容
    fseek(fp, 0, SEEK_END); // 定位到文件末尾
    int len = ftell(fp); // 获取文件长度
    fseek(fp, 0, SEEK_SET); // 定位到文件开头
    char* json_str = (char*)malloc(len + 1); // 分配内存
    if (json_str == NULL) {
        perror("malloc error");
        return -1;
    }
    memset(json_str, 0, len + 1); // 清空内存
    if (fread(json_str, len, 1, fp) != 1) { // 读取文件内容
        perror("fread sys_config error");
        return -1;
    }

    // 解析 JSON 字符串
    cJSON* root = cJSON_Parse(json_str);
    if (root == NULL) {
        perror("cJSON_Parse error");
        return -1;
    }

    // 解析 fps 对象
    cJSON* fps_obj = cJSON_GetObjectItem(root, "fps");
    if (fps_obj == NULL) {
        perror("cJSON_GetObjectItem fps error");
        return -1;
    }
    fps->frame_rate = cJSON_GetObjectItem(fps_obj, "frame_rate")->valueint;
    fps->sync_encoder = cJSON_GetObjectItem(fps_obj, "sync_encoder")->valueint;
    fps->priority = cJSON_GetObjectItem(fps_obj, "priority")->valueint;
    fps->net_flag = cJSON_GetObjectItem(fps_obj, "net_flag")->valueint;

    // 解析 window 对象
    cJSON* window_obj = cJSON_GetObjectItem(root, "window");
    if (window_obj == NULL) {
        perror("cJSON_GetObjectItem window error");
        return -1;
    }
    window->id = cJSON_GetObjectItem(window_obj, "id")->valueint;
    strcpy(window->src_ip, cJSON_GetObjectItem(window_obj, "src_ip")->valuestring);
    window->x = cJSON_GetObjectItem(window_obj, "x")->valueint;
    window->y = cJSON_GetObjectItem(window_obj, "y")->valueint;
    window->w = cJSON_GetObjectItem(window_obj, "w")->valueint;
    window->h = cJSON_GetObjectItem(window_obj, "h")->valueint;

    // 释放内存
    free(json_str);
    cJSON_Delete(root);
    fclose(fp);

    return 0;
}

int set_net_flag(fps_t* fps, unsigned char flag){
    if (flag != 0 && flag != 1) {
        perror("flag error");
        return -1;
    }
    fps->net_flag = flag;
    return 0;
}