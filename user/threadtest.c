#include "types.h"
#include "user.h"

const char* progs[] = {
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
  for (int i = 0; i < sizeof(progs); ++i) {
    exec(progs[i], "");
  }
  exit();
}
