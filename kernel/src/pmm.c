#include <common.h>

#ifndef TEST

#include <lock.h>
#include <queue.h>
extern uintptr_t que_physical_start;
extern uintptr_t que_physical_end;
extern spare_queue_t *q;

extern int print_lock;

static void *kalloc(size_t size) {
  assert(size > 0);
  // scope_t scope;
  if(size > (MAX_SIZE)){
    panic("超过 16 MiB 的内存分配, 丑拒!");
  }

    /* Queue_Dequeue 单元测试*/
  #ifdef Unit_Test_Queue_Dequeue
    printf("\n\n----------------------------Queue_Dequeue_Unit_Test--------------------------\n");
    uintptr_t start;
    Queue_Dequeue(q, &start, MAX_SIZE);
    printf("kalloc start = %p\n", start);
    printf("----------------------------Queue_Dequeue_Unit_Test--------------------------\n\n");
    return NULL;
  #else
    #ifdef verbose_on
      lock_acquire(&print_lock);
      printf("CPU#%d#: skip Queue_Enqueue_Unit_Test.\n", cpu_current());
      lock_release(&print_lock);
    #endif 
  #endif 

    /* Queue_Enqueue 单元测试*/
  #ifdef Unit_Test_Queue_Enqueue
    printf("\n\n----------------------------Queue_Enqueue_Unit_Test--------------------------\n");
    uintptr_t start;
    Queue_Dequeue(q, &start, MAX_SIZE);
    Queue_Enqueue(q, start + MAX_SIZE/2, MAX_SIZE/2);
    
    printf("kalloc start = %p\n", start);
    printf("----------------------------Queue_Enqueue_Unit_Test--------------------------\n\n");
  #else
    #ifdef verbose_on
      lock_acquire(&print_lock);
      printf("CPU#%d#: skip Queue_Enqueue_Unit_Test.\n", cpu_current());
      lock_release(&print_lock);
  #endif

#endif

    uintptr_t start = 0x0;
    size += sizeof(size_t); // 前8B存放大小
    size_t align_size = 1 << (log2(size) + 1);
    lock_acquire(&print_lock);
    printf("CPU#%d#: align_size = %x\n", cpu_current(), align_size);
    lock_release(&print_lock);

    /* 找到最小存在块 */
    size_t min_size = align_size;
    for (; min_size <= MAX_SIZE; min_size *= 2)
    {
      assert(min_size > 0);
      if (Queue_Dequeue(q, &start, min_size) != -1)
        break;
    }

    if (min_size > MAX_SIZE || start + align_size > (uintptr_t)heap.end)
    {

      return NULL;
    }

    min_size /= 2;
    /* 切分最小存在块 */
    for (; min_size >= align_size; min_size /= 2)
    {
      Queue_Enqueue(q, start + min_size, min_size);
    }

    assert(start >= que_physical_end && start + (uintptr_t)align_size <= (uintptr_t)heap.end);

#ifdef verbose_on
    lock_acquire(&print_lock);
    printf("CPU#%d#: [%p, %p) kalloc \n", cpu_current(), start, start + (uintptr_t)align_size);
    lock_release(&print_lock);
#endif
    *(size_t *)start = align_size;
    print_spare_queue(q);

    return (void *)(start + sizeof(size_t));
}

static void kfree(void *ptr) {
  
  uintptr_t start = (uintptr_t)ptr - (uintptr_t)sizeof(size_t);
  size_t size =  *(size_t*)start;
  memset((void*)start, 0, size);

  // char tmp[0x100] = {0};
  // sprintf(tmp, "CPU#%d#: kfree ptr = %p\n", cpu_current() , ptr);
  // printf("%s", tmp);
  lock_acquire(&print_lock);
  printf( "CPU#%d#: kfree ptr = %p\n", cpu_current() , ptr);
  lock_release(&print_lock);
  uintptr_t new_start = Queue_Merge(q, start, size);
  

  while(new_start != 0){
    start = new_start;
    size *= 2;
    if(size >= MAX_SIZE) break;
    new_start = Queue_Merge(q, start, size);
  } 
  
  Queue_Enqueue(q, start, size);
  print_spare_queue(q);
}

static void pmm_init() {

  #ifdef verbose_on
    printf("sizeof(size_t) = %d\n\n", sizeof(size_t));
  #endif
  

  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  Queue_Init(&q); 
  
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};

#else
#define HEAP_SIZE 1024 * 1024
Area heap = {};

// static void alloc(size_t size){
//   printf("alloc %ld size\n", size);
// }

// static void free(void *ptr){}

static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = malloc,
  .free  = free,
};
#endif
