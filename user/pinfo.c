#include "types.h"
#include "stat.h"
#include "pstat.h"
#include "user.h"

int
main()
{
  struct pstat stats;
  if (getpinfo(&stats) < 0)
    exit();

  for (int i = 0; i < NPROC; ++i) {
    if (!stats.inuse[i]) continue;
    printf(1, "------------------------\n");
    printf(1, "proc %d (pid: %d)\n", i, stats.pid[i]);
    printf(1, "------------------------\n");
    printf(1, "Tickets:         %d\n", stats.tickets[i]);
    printf(1, "Stride:          %d\n", stats.stride[i]);
    printf(1, "Pass:            %d\n", stats.pass[i]);
    printf(1, "Times scheduled: %d\n", stats.scheduled[i]);
    printf(1, "CPU Time:        %d\n", stats.ticks[i]);
    printf(1, "------------------------\n");
    printf(1, "\n");
  }

  exit();
}