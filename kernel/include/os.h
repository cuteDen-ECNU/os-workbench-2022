#ifndef OS_H__
#define OS_H__ 
#include <common.h>

struct task {
  struct task *next;
  const char *name;
  void   (*entry)(void *);
  Context  *context;
  uint8_t *stack;
  enum {
    READY,                            
    RUNNING,
    NEVER_SCHEDUlE,
  }status;
};

struct spinlock {
  const char *name;
  enum{
    LOCK,
    UNLOCK,
  }lock;
};

struct semaphore {
  // TODO
};
#endif