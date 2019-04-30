#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "param.h"

int
thread_create(void(*start_routine)(void*), void* arg)
{
  void *const stack = malloc(PGSIZE);
  return clone(start_routine, arg, stack);
}

int
thread_join(void)
{
  int pid;
  void* stack = NULL;

  pid = join(&stack);
  free(stack);
  return pid;
}

void
lock_init(lock_t *lock)
{
  lock->locked = 0;
}

void
lock_acquire(lock_t *lock)
{
  while(xchg(&lock->locked, 1) != 0);
}

void lock_release(lock_t *lock)
{
  xchg(&lock->locked, 0);
}