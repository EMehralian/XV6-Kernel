#include "types.h"
#include "stat.h"
#include "user.h"
#define CHILD_NUM 5
void
child(int pid)
{
  int i;
  for (i=0;i<100;i++)
    printf(1, "Child: %d prints for the : %d  time\n",pid,i);
}

void
rrSanity(void)
{
  int pid;
  printf(1, "wait test\n");
  int i=0;
    while(i<CHILD_NUM){
        pid = fork();
        if(pid==0)
        {
            child(getpid());
            exit();
        } else if(pid>0) {
            i++;
            printf(1,"Parent\n");
        } else{
            printf(1,"Error is fork\n");
        }
    }
    int k=0;
    for(k=0;k<CHILD_NUM;k++) {
        wait();
    }
    printf(1,"Parent finished work\n");
    int numOfChilds=getChildSize();
    printf(1,"childsize:%d\n",numOfChilds);
    int j=0;
    for ( j= 0; j <numOfChilds ; j++) {
        int cTime=0;
        int rTime=0;
        int eTime=0;
        cTime=getAllChildsCtime(j);
        rTime=getAllChildsRtime(j);
        eTime=getAllChildsEtime(j);
        printf(1,"Child %d perfomance is:rTime:%d:eTime:%d:cTime:%d\n",j,rTime,eTime,cTime);
    }
  }

int
main(void)
{
  rrSanity();
  exit();
}