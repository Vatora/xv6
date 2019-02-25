#include "types.h"
#include "stat.h"
#include "pstat.h"
#include "user.h"

#define DEFAULT_CHILD_COUNT 3
#define DEFAULT_SLEEP_TIME 300

// Print the help message and exit
void
print_help(int invalid_args)
{
  if (invalid_args)
    printf(1, "Invalid arguments\n");
  else
    printf(1, "ticktest: Test the scheduler by comparing the time allocation for processes with different numbers of tickets\n");

  printf(1, "\n");
  printf(1, "Usage:\n");
  printf(1, "-h  Print this help message\n");
  printf(1, "-p  Number of processes to run\n");
  printf(1, "-t  Amount of time to wait for processes to accumulate CPU time (units: 0.01s)\n");

  exit();
}

int
main(int argc, char** argv)
{
  int  sleep_time  = DEFAULT_SLEEP_TIME;
  int  child_count = DEFAULT_CHILD_COUNT;
  int* pids;

  // Get the arguments
  if (argc < 2) {
    print_help(1);
  }
  else {
    for (int i = 1; i < argc; ++i) {

      // Help
      if (strcmp(argv[1], "-h") == 0) {
       print_help(0);
      }

      // Process count
      if (strcmp(argv[i], "-p") == 0) {
        if (i == (argc-1)) {
          print_help(1);
        }
        else {
          child_count = atoi(argv[i+1]);
          i++;
          continue;
        }
      }

      // Sleep time
      if (strcmp(argv[i], "-t") == 0) {
        if (i == (argc-1)) {
          print_help(1);
        }
        else {
          sleep_time = atoi(argv[i+1]);
          i++;
          continue;
        }
      }
    }
  }

  // Allocate the PID array
  pids = malloc(child_count * sizeof(int));

  // Start each child process
  for (int i = 0; i < child_count; ++i) {
    pids[i] = fork();
    if (pids[i] == 0) {
      setticket((i+1) * 10);
      for (;;)
        ;
    }
  }

  // Sleep so the children can accumulate CPU time
  sleep(sleep_time);

  // Get the process info
  struct pstat stats;
  if (getpinfo(&stats) < 0) {
	printf(2, "Failed to get pstat\n");
    free(pids);
    exit();
  }

  // End the child processes
  for (int i = 0; i < child_count; ++i) {
    kill(pids[i]);
  }
  for (int i = 0; i < child_count; ++i) {
    wait();
  }

  // Print the stats for the children
  printf(1, "\n");
	for (int i = 0; i < child_count; ++i) {
		for (int j = 0; j < NPROC; ++j) {
			if (pids[i] == stats.pid[j]) {
				printf(1, "Process %d (pid: %d)\n", i, pids[i]);
				printf(1, "-----------------------\n");
				printf(1, "Tickets:         %d\n", stats.tickets[j]);
				printf(1, "Times Scheduled: %d\n", stats.scheduled[j]);
				printf(1, "Ticks:           %d\n", stats.ticks[j]);
        printf(1, "\n");
			}
		}
	}

  free(pids);
  exit();
}