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