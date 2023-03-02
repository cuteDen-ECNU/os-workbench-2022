
#ifndef QUEUE_H__
#define QUEUE_H__

// #include <stdlib.h>
#include <klib.h>

#define MAX_SIZE (1<<24)

#define MAP_BITS 0x10000 
#define FREELIST_NUM 24


/* 
  队列的api 
  队列的每一个节点都是 16 MiB 的倍数
*/

typedef struct __node_t
{
    uintptr_t _start, _end;
    struct __node_t *next;
}spare_node_t;

typedef struct __bit_t{
    unsigned value : 1;
}bit;

typedef struct __queue_t
{ 
    bit bit_map[MAP_BITS];

    spare_node_t *head[FREELIST_NUM];
    spare_node_t *tail[FREELIST_NUM];
    int bit_lock, head_lock[FREELIST_NUM], tail_lock[FREELIST_NUM];
}spare_queue_t;


void setScope(spare_node_t *p, uintptr_t start, uintptr_t end);

int log2(size_t size);

spare_node_t* kalloc_node(spare_queue_t *q);

void kfree_node(spare_queue_t *q, spare_node_t *pNode);

void print_spare_queue(spare_queue_t* q);

void set_que_physical();

void Queue_Init(spare_queue_t **pQue);

void Queue_Enqueue(spare_queue_t *q, uintptr_t start, size_t size);

int Queue_Dequeue(spare_queue_t *q, uintptr_t* start, size_t size);

uintptr_t Queue_Merge(spare_queue_t *q, uintptr_t start, size_t size);

#endif