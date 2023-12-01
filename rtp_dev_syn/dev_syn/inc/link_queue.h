#ifndef __LINK_QUEUE_H__
#define __LINK_QUEUE_H__

#include "common.h"
#include <arpa/inet.h> // struct sockaddr_in

// 链式队列结点
typedef struct link_queue_node {
    struct sockaddr_in cmd_addr; // 指令发送端地址
    char cmd[CMD_BUF_SIZE]; // 指令
    struct link_queue_node* next;
} node_t;

// 链式队列
typedef struct link_queue {
    node_t* front;
    node_t* rear;
} link_queue_t;

/**
 * @brief 创建链式队列
 *
 * @return link_queue_t* 链式队列指针
 */
link_queue_t* link_queue_create(void);

/**
 * @brief 销毁链式队列
 *
 * @param queue 链式队列指针
 * @return int 0: 成功, -1: 失败
 */
int link_queue_destroy(link_queue_t* queue);

/**
 * @brief 判断链式队列是否为空
 *
 * @param queue 链式队列指针
 * @return int 1: 空, 0: 非空，-1: 队列不存在
 */
int link_queue_is_empty(link_queue_t* queue);

/**
 * @brief 入队
 *
 * @param queue 链式队列指针
 * @param cmd_addr 指令发送端地址
 * @param cmd 命令
 * @return int 0: 成功, -1: 失败
 */
int link_queue_enqueue(link_queue_t* queue, struct sockaddr_in* cmd_addr, const char* cmd);

/**
 * @brief 出队
 *
 * @param queue 链式队列指针
 * @param cmd 命令
 * @return int 0: 成功, -1: 失败
 */
int link_queue_dequeue(link_queue_t* queue, struct sockaddr_in* cmd_addr, char* cmd);

#endif // __LINK_QUEUE_H__