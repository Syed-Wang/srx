#include "sys_config.h"
#include "cJSON.h"
#include "tool.h"
#include <stdio.h>
#include <stdlib.h> // malloc()
#include <string.h> // memset()

fps_t fps; // 帧率结构体
window_t window; // 窗口结构体

int save_sys_config(fps_t* fps, window_t* window)
{
    FILE* fp = fopen(SYS_CONFIG_PATH, "w"); // w 清空再写入
    if (fp == NULL) {
        PTR_PERROR("fopen sys_config error");
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

    // 添加 ip 数组到 fps 对象 (类型：char ip[128][16])
    cJSON* ip_array = cJSON_CreateArray(); // 创建 ip 数组
    for (int i = 0; i < node_num; i++) {
        cJSON_AddItemToArray(ip_array, cJSON_CreateString(fps->ip[i])); // 添加 ip 到 ip 数组
    }
    cJSON_AddItemToObject(fps_obj, "ip", ip_array); // 添加 ip 数组到 fps 对象

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
        // PTR_PERROR("fopen sys_config error");
        printf("The file %s does not exist, create it.\n", SYS_CONFIG_PATH);
        return -2; // 文件不存在
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
    // 解析 ip 数组到 fps 对象 (类型：char ip[128][16])
    memset(fps->ip, 0, sizeof(fps->ip)); // 清空内存
    cJSON* ip_array = cJSON_GetObjectItem(fps_obj, "ip");
    if (ip_array == NULL) {
        perror("cJSON_GetObjectItem ip error");
        return -1;
    }
    node_num = cJSON_GetArraySize(ip_array); // 获取 ip 数组长度
    for (int i = 0; i < node_num; i++) {
        strcpy(fps->ip[i], cJSON_GetArrayItem(ip_array, i)->valuestring); // 获取 ip 数组元素
    }

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
