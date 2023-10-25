#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "pstat.h"
#include "spinlock.h"



int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


int
sys_settickets(void)
{
  int total_ticket = 0;
  // int pid;

  // if(argint(0, &pid) < 0)
  //   return -1;
  // return kill(pid);
  if(argint(0, &total_ticket) < 0){
    return -1;
  }
  acquire(&ptable.lock);
  setp_tickets(ptable.proc, total_ticket);
  release(&ptable.lock);
  return 0;
}

int
sys_getpinfo(void)
{
  struct pstat* pp;
  struct proc* p;
    // if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    // return -1;
  // if(argptr(0,(void*)&pp,sizeof(&pp)) <0){
  //   return -1;
  // }
  argptr(0, (void*)&pp, sizeof(pp));
  if(pp == NULL){
    panic("PP's gone!\n");
    return 1;
   //cprintf("There is no current processes, returning");
  }
  acquire(&ptable.lock);
  // set_tickets(ptable.proc, total_ticket);
  //loop over all proc
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    int i = p - ptable.proc;
    if(p->state != UNUSED){
      pp->pid[i] = p->pid;
      pp->ticks[i] = p->ticks;
      pp->tickets[i] = p->tickets;
      pp->inuse[i] = p->inuse;
    }
  }
  release(&ptable.lock);
  return 0;
}