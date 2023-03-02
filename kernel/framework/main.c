#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <lock.h>
#include <unistd.h>
#define CAPACITY 0x100000

typedef struct __CircularQueue_t
{
  int head, tail;
  int lock;
  void* malloced_pointer[CAPACITY];
}CircularQueue;

CircularQueue cq;

enum ops { OP_ALLOC = 1, OP_FREE };
struct malloc_op {
  enum ops type;
  union { size_t sz; void *addr; };
};

int print_lock = 0;
void print_queue(){
  lock_acquire(&cq.lock);
  char out[0x100] = {0};
  sprintf(out, "CPU#%d#: cq.head = %d, cq.tail = %d\t", cpu_current(), cq.head, cq.tail);
  // printf("CPU#%d#:【print_queue】 cq.head = %d, cq.tail = %d\t", cpu_current(), cq.head, cq.tail);
  if (cq.head == (cq.tail + 1) % CAPACITY){
    lock_release(&cq.lock);
    lock_acquire(&print_lock);
    printf("%s\n", out);
    lock_release(&print_lock);
    return;
  }
  
  int cur = cq.head;

  while(1){
    char addrs[0x100] = {0};
    sprintf(addrs, "CPU#%d#: cq.head = %d, cq.tail = %d\t", cpu_current(), cq.head, cq.tail); 

    for(int i = 0; i < 20; ++i){
      if (cur == (cq.tail + 1) % CAPACITY)continue;
      sprintf(addrs, "%s%p ", addrs, cq.malloced_pointer[cur]); 
      cur = (cur + 1) % CAPACITY;
    }

    lock_acquire(&print_lock);
    printf("%s\n", addrs);
    lock_release(&print_lock); 

    if (cur == (cq.tail + 1) % CAPACITY) break;
  }

  
  lock_release(&cq.lock);

}

void init_queue(){
  cq.head = 0; cq.tail = -1;
}

struct malloc_op random_op(){
  struct malloc_op op;
  int type = rand()%2 + 1;
  // int type = OP_ALLOC;

  lock_acquire(&cq.lock); 
  if(type == OP_ALLOC || cq.head == (cq.tail + 1) % CAPACITY){
    op.type = OP_ALLOC;
    int i = rand()%20;
    op.sz = 1 << i;
    lock_release(&cq.lock);
    return op;
  }else{
    assert(cq.malloced_pointer[cq.head] != NULL);
    op.type = OP_FREE;
    
    op.addr = cq.malloced_pointer[cq.head];
    cq.malloced_pointer[cq.head] = NULL; 
    cq.head = (cq.head + 1) % CAPACITY;
    lock_release(&cq.lock);
    return op;
  }
  
}

void stress_test() {

  lock_acquire(&print_lock);
  printf("CPU#%d#: stress test.\n",cpu_current());
  lock_release(&print_lock);
  
  while (1) {
    // if(cpu_current()){for(int i = 0; i < 20; ++i)print("\n");} //加延迟
    struct malloc_op op = random_op();

    lock_acquire(&print_lock);
    printf("CPU#%d#: op.type = %d, op.addr = %p\n", cpu_current(), op.type, op.addr);
    lock_release(&print_lock);

    switch (op.type) {
      case OP_ALLOC:
      {
        void* ptr = pmm->alloc(op.sz);
        if(ptr != NULL){
          lock_acquire(&cq.lock);
          cq.tail = (cq.tail + 1) % CAPACITY; 
          cq.malloced_pointer[cq.tail] = ptr; 
          lock_acquire(&print_lock);
          printf("CPU#%d#: main alloc ptr = %p, size = 0x%x\n", cpu_current(), ptr, op.sz);
          lock_release(&print_lock);
          lock_release(&cq.lock);
        }
        // for (int volatile i = 0; i < 100000; i++) ;
        
        print_queue();
        break;
      } 
      case OP_FREE: 
      { 
        lock_acquire(&print_lock);
        printf("CPU#%d#: main free ptr = %p\n", cpu_current(), op.addr);
        lock_release(&print_lock);
        pmm->free(op.addr);
        print_queue();
        break;
      } 
    }
  }
}

int main() {
  os->init();
  init_queue();
  mpe_init(stress_test);
  return 1;
}
