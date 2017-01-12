#include "types.h"
#include "stat.h"
#include "user.h"

void
child(int pid)
{
  int i;
  for (i=0;i<5;i++)
    printf(1, "Child: %d prints for the : %d  time\n",pid,i);
}

void
rrSanity(void)
{
  int wTime;
  int rTime;
  int pid;
  printf(1, "wait test\n");
  int ppid=getpid();
  for(int i=0 ; i<10;i++){
    pid = fork();
    if(pid != ppid)
    {
      child(pid);
      getPerformanceData(&wTime,&rTime);
      printf(1, "hi \n");
      printf(1, "wTime: %d rTime: %d \n",wTime,rTime);
      exit();      
    }
  wait();    
  }
}

int
main(void)
{
  rrSanity();
  exit();
}