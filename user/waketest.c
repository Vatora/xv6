#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int rc = fork();
  if (rc == 0) {  // Child process
    setticket(50);  // Give Child higher priority
    sleep(100);  // Sleep for 100ms (? units unknown)
    for (int i = 0; i < 500; i++) {
      printf(1, "Child PID %d: %d\n", getpid(), i);
    }
  } 
  else {  // Parent process
    for (int i = 0; i < 500; i++) {
      printf(1, "Parent PID %d: %d\n", getpid(), i);
    }
    wait(); // Wait for child to finish
  }
  exit();
}
