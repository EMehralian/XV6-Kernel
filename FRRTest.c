//
// Created by mohammad on 1/15/17.
//
#include "types.h"
#include "stat.h"
#include "user.h"

#define CHILD_NUM 4
#define LOOP_COUNT 100000

void FRRTest() {
    enableContextSwitchPrint(1);
    int pid;
    printf(1, "FRRTest paretId is:%d\n",getpid());
    int i = 0;
    while (i < CHILD_NUM) {
        pid = fork();
        if (pid == 0) {
            int j;
            for (j = 0; j < LOOP_COUNT; j++);
            printf(1, "Child %d finished\n",getpid());
            exit();
        } else if (pid > 0) {
            i++;
            printf(1, "Parent\n");
        } else {
            printf(1, "Error is fork\n");
        }
    }
    int k = 0;
    for (k = 0; k < CHILD_NUM; k++) {
        wait();
    }
    printf(1, "Parent finished work\n");
    enableContextSwitchPrint(0);
}

int main(void) {
    FRRTest();
    exit();
}
