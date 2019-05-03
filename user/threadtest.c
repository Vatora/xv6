#include "types.h"
#include "user.h"

char *const progs[] = {
	"T_badclone",
	"T_clone",
	"T_clone2",
	"T_clone3",
	"T_join",
	"T_join2",
	"T_join3",
	"T_join4",
	"T_locks",
	"T_multi",
	"T_noexit",
	"T_size",
	"T_stack",
	"T_thread",
	"T_thread2",
};


int
main()
{
  char *args[] = {"test", 0};
  for (int i = 0; i < (sizeof(progs) / sizeof(progs[0])); ++i) {
    if (fork() == 0) {
      printf(1, "executing %s... ", progs[i]);
      if (exec(progs[i], args) < 0) {
        printf(1, "exec failed\n", progs[i]);
      }
      exit();
    }
    wait();
  }
  exit();
}
