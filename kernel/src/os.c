#include <common.h>

static void os_init() {
  pmm->init();
}

static void os_run() {
#ifndef TEST
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   putch(*s == '*' ? '0' + cpu_current() : *s);
  // }
   
  for(int i = 0; i < 100; ++i){
    void* p = pmm->alloc(0x10000);
    if(p == NULL){
      printf("CPU#%d#: malloc failed at i = %d\n", i, cpu_current());
    }else{
      printf("CPU#%d#: malloc success at p = %p, size = 0x%x\n", cpu_current(), p, *(size_t*)((uintptr_t)p - (uintptr_t)sizeof(size_t)));
    }
  }

  while (1) ;
#endif
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
