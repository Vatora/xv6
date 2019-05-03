#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"
#include "proc_queue.h"

#define MAX_QUANTA 10

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  proc_queue pqueue;
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
static void set_min_pass(struct proc* newproc);

void
getpstats1(struct pstat* stats)
{
  for (int i = 0; i < NPROC; ++i) {
    stats->inuse[i]     = ptable.proc[i].state != UNUSED;
    stats->pid[i]       = ptable.proc[i].pid;
    stats->tickets[i]   = ptable.proc[i].schdldat.tickets;
    stats->stride[i]    = ptable.proc[i].schdldat.stride;
    stats->pass[i]      = ptable.proc[i].schdldat.pass;
    stats->scheduled[i] = ptable.proc[i].schdldat.schdlnum;
    stats->ticks[i]     = ptable.proc[i].schdldat.schdlnum * 10; //Assuming 10ms quantum w/ no early interrupt
  }
}

void
getpstats(struct pstat* stats)
{
  if (holding(&ptable.lock)) {
    getpstats1(stats);
  }
  else {
    acquire(&ptable.lock);
    getpstats1(stats);
    release(&ptable.lock);
  }
}

// Assumes ptable.lock has already been acquired
static void
set_min_pass(struct proc* newproc)
{
  struct proc* pmin = proc_queue_peek_min(&ptable.pqueue);
  if (!newproc || !pmin)
    return;

  // Number of scheduler quanta for which the new process will occupy the CPU
  const int pass_delta = pmin->schdldat.pass - newproc->schdldat.pass;
  const int quanta     = pass_delta / newproc->schdldat.stride;

  // Make the process take at most MAX_QUANTA scheduler quanta
  // before a different process is scheduled
  if (quanta > MAX_QUANTA)
    newproc->schdldat.pass = pmin->schdldat.pass - (MAX_QUANTA * newproc->schdldat.stride);
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  proc_queue_init(&ptable.pqueue);
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  // Initialize the sheduling data
  p->schdldat.tickets = DEFAULT_TICKETS;
  p->schdldat.stride = STRIDE_DIV / DEFAULT_TICKETS;
  p->schdldat.pass = 0;
  p->schdldat.schdlnum = 0;

  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

void
deallocproc(struct proc *p)
{
  kfree(p->kstack);
  p->stack = 0;
  p->kstack = 0;
  if (p->pgdir != p->parent->pgdir) //if not a thread
    freevm(p->pgdir);
  p->state = UNUSED;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE * 2;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE * 2;
  p->tf->eip = PGSIZE;  // beginning of initcode.S
  p->stack = 0;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  proc_queue_insert(&ptable.pqueue, p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;

  // Update the sizes of child threads.
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; ++p) {
	  if (p->parent == proc && p->pgdir == proc->pgdir) {
		  p->sz = sz;
	  }
  }
  release(&ptable.lock);

  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Copy the scheduling data from the parent
  np->schdldat.tickets = proc->schdldat.tickets;
  np->schdldat.stride = proc->schdldat.stride;
  np->schdldat.pass = proc->schdldat.pass;


  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));

  acquire(&ptable.lock);
  proc_queue_insert(&ptable.pqueue, np);
  release(&ptable.lock);

  return pid;
}

int
clone(void(*fcn)(void*), void *arg, void *stack)
{
  int i, pid;
  uint sp;
  uint ustack[2];
  struct proc *np;

  // Validate page size and alignment.
  if ((proc->sz - (uint)stack) < PGSIZE ||
      (uint)stack % PGSIZE != 0) {
    return -1;
  }

  // Allocate a new process
  if ((np = allocproc()) == 0) {
    return -1;
  }

  // Copy process state from p.
  np->pgdir = proc->pgdir;
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  // Initial stack contents (as seen in exec.c)
  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = (uint)arg;

  // Setup stack.
  sp = (uint)stack + PGSIZE; //stack is one page
  sp -= sizeof(ustack);
  if (copyout(np->pgdir, sp, ustack, sizeof(ustack)) < 0) {
    return -1;
  }
  np->tf->esp = sp;
  np->stack = stack;

  // Set the thread's instruction pointer
  np->tf->eip = (uint)fcn;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Wait for threads spawned by proc to finish execution.
// Very similar to wait().
int
join(void **stack)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

      // Find a thread spawned by this process
      if(p->parent != proc || p->pgdir != proc->pgdir)
        continue;
        
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        *stack = p->stack; //set the stack address
        deallocproc(p);
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      if (p->pgdir == proc->pgdir)
        deallocproc(p);
      else
        p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if (p->parent != proc)
        continue;
      if (p->pgdir == proc->pgdir)
        continue;

      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        deallocproc(p);
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    // Run the process with the minimum pass value
    p = proc_queue_pop_min(&ptable.pqueue);

    if (p && p->state == RUNNABLE) {
      p->schdldat.pass += p->schdldat.stride;
      p->schdldat.schdlnum++;
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    else if (p) {
        cprintf("non-runnable process in queue: %s (0x%p) (state: %d)\n", p->name, p, p->state);
    }
    
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  proc_queue_insert(&ptable.pqueue, proc);
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      set_min_pass(p);
      proc_queue_insert(&ptable.pqueue, p);
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
        proc_queue_insert(&ptable.pqueue, p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


