#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_getppid(void)
{
  // return proc->sz;
  // return 5;
  // printf("Hello");
   return proc->parent->pid+6;
}

int
sys_getPerformanceData(void)
{
 
  char *wtime ;
  char *rtime;
// argstr(int n, char **pp)

  argstr(0, &wtime);
  argstr(1, &rtime);

  *wtime = proc->etime-(proc->ctime)-(proc->rtime);
  *rtime = proc->rtime;
  return 1;
}


int sys_calculatePerfomance(int rtime,int ctime,int etime,int* wAndrTime){
    wAndrTime[0] = etime-(ctime)-(rtime);
    wAndrTime[1] = rtime;
    return 1;
}

int sys_getAllChildsRtime(void){
    int childAt=-1;
    argint(0,&childAt);
    return proc->childRTime[childAt];
}

int sys_getAllChildsEtime(void){
    int childAt=-1;
    argint(0,&childAt);
    return proc->childETime[childAt];
}

int sys_getAllChildsCtime(void){
    int childAt;
    argint(0,&childAt);
    return proc->childCTime[childAt];
}

int sys_getChildSize(void)
{
    return proc->allChildSize;
}