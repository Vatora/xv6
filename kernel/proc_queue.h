#ifndef _PROC_QUEUE_H_
#define _PROC_QUEUE_H_

#include "param.h"

struct proc;


//----------------------------------------------------------------------------------
// Binary Heap
//----------------------------------------------------------------------------------
typedef struct bin_heap_node {
  int value;
  void* data;
} bin_heap_node;

typedef struct binary_heap {
  bin_heap_node nodes[NPROC];
  int size;
  int max_size;
} binary_heap;


//----------------------------------------------------------------------------------
// Process Queue
//----------------------------------------------------------------------------------
//
// A simple wrapper around the binary heap that works with the xv6 process struct.
//
//----------------------------------------------------------------------------------
typedef struct proc_priority_queue {
  binary_heap heap;
} proc_queue;

void proc_queue_init(proc_queue* queue);
void proc_queue_rebuild(proc_queue* queue);
void proc_queue_insert(proc_queue* queue, struct proc* p);
struct proc* proc_queue_peek_min(const proc_queue* queue);
struct proc* proc_queue_pop_min(proc_queue* queue);
void proc_queue_print(const proc_queue* queue);

#endif //_PROC_QUEUE_H_