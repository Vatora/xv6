#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

// Stats for all extant processes

struct pstat {
  int   inuse[NPROC];      // Whether this slot of the process table is in use
  int   pid[NPROC];        // PID of each process
  int   tickets[NPROC];    // Number of tickets this process has
  int   stride[NPROC];     // Stride of each process calculated from the number of tickets
  int   pass[NPROC];       // Current pass value of each process
  int   scheduled[NPROC];  // Number of times each process has been shceduled
  int   ticks[NPROC];      // Number of ticks each process has accumulated
};

#endif // _PSTAT_H_
