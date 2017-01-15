//
// Created by mohammad on 1/15/17.
//
#include "types.h"
#include "stat.h"
#include "user.h"

#define LOOP_COUNT 50

void printProcess(int pid){
    int i=0;
    for(i=0;i<LOOP_COUNT;i++){
        printf(1,"Process %d is printing for %d time\n",pid,i);
    }
}

void Gsanity() {
    int pid;
    printf(1, "Gsanity pId is:%d\n",getpid());
    sleep(1000);
    pid = fork();
    if (pid == 0) {
        nice();
        nice();
        printProcess(getpid());
        printf(1, "Child %d finished\n",getpid());
        exit();
    } else if (pid > 0) {
        nice();
        nice();
        printProcess(getpid());
        printf(1, "Parent\n");
    } else {
        printf(1, "Error is fork\n");
    }
    printf(1, "Parent finished work\n");
}

int main(void) {
    Gsanity();
    exit();
}
