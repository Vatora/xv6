#include "proc_queue.h"
#include "types.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "defs.h"

static void bin_heap_init(binary_heap* heap);
static void bin_heap_heapify(binary_heap* heap);
static void bin_heap_insert(binary_heap* heap, void* data, int value);
static void bin_heap_bubble_down(binary_heap* heap, int idx);
static void bin_heap_bubble_up(binary_heap* heap, int idx);
static void bin_heap_delete_min();
static const bin_heap_node* bin_heap_peek_min(const binary_heap* heap);
static bin_heap_node bin_heap_pop_min(binary_heap* heap);

static const int g_int_max = (((1 << (sizeof(int)*8 - 2)) - 1) * 2) + 1;


//----------------------------------------------------------------------------------
// Process Queue
//----------------------------------------------------------------------------------

void
proc_queue_init(proc_queue* queue)
{
	bin_heap_init(&queue->heap);
}


void
proc_queue_rebuild(proc_queue* queue)
{
	bin_heap_heapify(&queue->heap);
}

struct proc*
proc_queue_peek_min(const proc_queue* queue)
{
	const bin_heap_node* node = bin_heap_peek_min(&queue->heap);
  return node ? node->data : NULL;
}

void
proc_queue_insert(proc_queue* queue, struct proc* p)
{
	if (p) {
		//cprintf("push {v:%d, d:%p}\n", p->schdldat.pass, p);
		bin_heap_insert(&queue->heap, p, p->schdldat.pass);
	}
}

struct proc*
proc_queue_pop_min(proc_queue* queue)
{
	const bin_heap_node node = bin_heap_pop_min(&queue->heap);
	return node.data; 
}

void
proc_queue_print(const proc_queue* queue)
{
	cprintf("|| size: %d |", queue->heap.size);
	for (int i = 0; i < queue->heap.size; i++) {
		const bin_heap_node* node = &queue->heap.nodes[i];
		cprintf("| %s (%d) ", ((struct proc*)node->data)->name, node->value);
	}
	cprintf("||\n");
}




//----------------------------------------------------------------------------------
// Binary Heap
//----------------------------------------------------------------------------------

static void
bin_heap_init(binary_heap* heap)
{
	heap->size = 0;
	heap->max_size = NPROC;
	for (int i = 0; i < NPROC; i++) {
		heap->nodes[i].value = g_int_max;
		heap->nodes[i].data = NULL;
	}
}

static void
bin_heap_heapify(binary_heap* heap)
{
	for (int i = heap->size - 1; i >= 0; i--) {
		bin_heap_bubble_down(heap, i);
	}
}

static void
bin_heap_bubble_down(binary_heap* heap, int idx)
{
	if (!heap)
		return;

	const int left_idx  = 2*idx + 1;
	const int right_idx = 2*idx + 2;

	if (left_idx >= heap->size)
		return;

	int min_idx = idx;

	if (heap->nodes[idx].value > heap->nodes[left_idx].value) {
		min_idx = left_idx;
	}

	if ((right_idx < heap->size) && (heap->nodes[min_idx].value > heap->nodes[right_idx].value)) {
		min_idx = right_idx;
	}

	if (min_idx != idx) {
		bin_heap_node temp = heap->nodes[idx];
		heap->nodes[idx] = heap->nodes[min_idx];
		heap->nodes[min_idx] = temp;
		bin_heap_bubble_down(heap, min_idx);
	}
}

static void
bin_heap_bubble_up(binary_heap* heap, int idx)
{
	if (!heap || idx == 0)
		return;

	const int parent_idx = (idx-1) / 2;

	if (heap->nodes[parent_idx].value > heap->nodes[idx].value) {
		bin_heap_node temp = heap->nodes[parent_idx];
		heap->nodes[parent_idx] = heap->nodes[idx];
		heap->nodes[idx] = temp;
		bin_heap_bubble_up(heap, parent_idx);
	}
}

static void
bin_heap_insert(binary_heap* heap, void* data, int value)
{
	if (!heap)
		return;

	heap->nodes[heap->size].value = value;
	heap->nodes[heap->size].data = data;
	bin_heap_bubble_up(heap, heap->size);
	heap->size++;
}

static void
bin_heap_delete_min(binary_heap* heap)
{
	if (!heap || heap->size == 0)
		return;

	heap->nodes[0] = heap->nodes[heap->size - 1];
	heap->size--;

	bin_heap_bubble_down(heap, 0);
}

static const bin_heap_node*
bin_heap_peek_min(const binary_heap* heap)
{
	return (heap && heap->size == 0) ? NULL : &heap->nodes[0];
}

static bin_heap_node
bin_heap_pop_min(binary_heap* heap)
{
  bin_heap_node out;
  const bin_heap_node* node = bin_heap_peek_min(heap);
  if (node) {
    out = *node;
    bin_heap_delete_min(heap);
    return out;
  }
  else {
    out.value = g_int_max;
    out.data = NULL;
    return out;
  }
}