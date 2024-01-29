#include "link_queue.h"
#include "tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

link_queue_t* link_queue_create(void)
{
    link_queue_t* queue = (link_queue_t*)malloc(sizeof(link_queue_t));
    if (queue == NULL) {
        PTRERR("malloc error");
        return NULL;
    }
    queue->front = queue->rear = NULL;
    return queue;
}

int link_queue_destroy(link_queue_t* queue)
{
    if (queue == NULL) {
        PTRERR("queue is NULL");
        return -1;
    }
    node_t* p = queue->front;
    while (p != NULL) {
        queue->front = p->next;
        free(p);
        p = queue->front;
    }
    free(queue);
    return 0;
}

int link_queue_is_empty(link_queue_t* queue)
{
    if (queue == NULL) {
        PTRERR("queue is NULL");
        return -1;
    }
    if (queue->front == NULL) {
        return 1;
    }
    return 0; // 非空
}

int link_queue_enqueue(link_queue_t* queue, struct sockaddr_in* cmd_addr, const char* cmd)
{
    if (queue == NULL) {
        PTRERR("queue is NULL");
        return -1;
    }

    node_t* p = (node_t*)malloc(sizeof(node_t));
    if (p == NULL) {
        PTRERR("malloc error");
        return -1;
    }
    p->cmd_addr = *cmd_addr;
    strcpy(p->cmd, cmd);
    p->next = NULL;

    if (queue->front == NULL) {
        queue->front = queue->rear = p;
    } else {
        queue->rear->next = p;
        queue->rear = p;
    }

    return 0;
}

int link_queue_dequeue(link_queue_t* queue, struct sockaddr_in* cmd_addr, char* cmd)
{
    if (link_queue_is_empty(queue) == 1) {
        PTRERR("queue is empty");
        return -1;
    } else if (link_queue_is_empty(queue) == -1) {
        PTRERR("queue is NULL");
        return -1;
    }

    node_t* p = queue->front;
    queue->front = p->next;
    *cmd_addr = p->cmd_addr;
    strcpy(cmd, p->cmd);
    free(p);
    p = NULL;

    return 0;
}
