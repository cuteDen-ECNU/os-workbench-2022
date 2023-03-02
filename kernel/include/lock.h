#ifndef LOCK_H__
#define LOCK_H__

#include <common.h>

#define PMMLOCKED 1
#define PMMUNLOCKED 0
/* 锁的api */

// 阻塞，直到获取锁
static void
lock_acquire(int *lock)
{
  while(atomic_xchg(lock, PMMLOCKED)) {;}
}

// 释放获取的锁
static void
lock_release(int *lock)
{
  atomic_xchg(lock, PMMUNLOCKED); 
}

#endif
