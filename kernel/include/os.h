#ifndef OS_H__
#define OS_H__ 
#include <common.h>

struct task {
  struct task *bk, *fd;
  const char *name;
  void   (*entry)(void *);
  Context  *context;
  uint8_t *stack;
  enum {                         
    RUNNING,
    RUNNABLE,
    BLOCKED,
    NEVER_SCHEDUlE,
  }status;
};

struct spinlock {
  const char *name;
  int locked;
  enum{
    UNLOCK,
    LOCK,
  }lock;
};

struct semaphore {
  
  int count;
  spinlock_t lock;
  task_t *wait_list;
};

struct mutexlock {
  int locked;
  spinlock_t lock;
  task_t *wait_list;
};

#endif