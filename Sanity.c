//
// Created by mohammad on 1/15/17.
//
#include "types.h"
#include "stat.h"
#include "user.h"

#define CHILD_NUM 30
#define LOOP_COUNT 50

void SanityTest() {
    int pid;
    printf(1, "Sanitytest parent Id is:%d\n", getpid());
    int i = 0;
    while (i < CHILD_NUM) {
        pid = fork();
        if (pid == 0) {
            int j;
            int piority=-1;
            int r = i%3;
            switch (r) {
                case 1:
                    nice();
                    piority=nice();
                    break;
                case 0:
                    piority=nice();
                    break;
                case 2:
                    piority=2;
                default:
                    break;
            }
            printf(1,"Child %d piority is:%d\n",i,piority);
            for (j = 0; j < LOOP_COUNT; j++) {
                printf(1, "< Child %d piority %d>\n", i,piority);
//                sleep(1000);
            }
            printf(1, "----Child %d finished\n",i);
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
}

int main(void) {
    SanityTest();
    exit();
}
