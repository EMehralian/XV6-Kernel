#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"

int main(void){
int a = getppid();
  printf(1,"The id of parent process is:%d", a);
  exit();
}
