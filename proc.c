#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

//MYCODE
#define RR          0      // Round Robin policy
#define FRR         1      // FIFO Round Robin policy
#define GRT         2      // Guaranteed (Fair-share) Scheduling policy
#define Q3          3      // Multi-Level Queue Scheduling policy

int policyChooser = Q3;

void sortProcessFIFO();

void printAllRunningProcesses();

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;
// struct proc procList[NPROC+1];
static struct proc *initproc;
int nextpid = 1;

//int newProcIsAdded=0;
extern void forkret(void);

extern void trapret(void);

static void wakeup1(void *chan);

void pinit(void) {
    initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void) {
    struct proc *p;
    char *sp;
    acquire(&ptable.lock);

//    newProcIsAdded = 1;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

    found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    p->rtime = 0;
    p->ctime = ticks; ///set create position of new process
    p->allChildSize = 0;
    //cprintf("================%d starting\n",p->ctime);
    if (policyChooser == FRR) {
        sortProcessFIFO();
    } else if (policyChooser == Q3) {
        p->piority = HIGH_PIORITY;//Highest one
    }
    release(&ptable.lock);
    // Allocate kernel stack.
    if ((p->kstack = kalloc()) == 0) {
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;
    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe *) sp;
    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint *) sp = (uint) trapret;
    sp -= sizeof *p->context;
    p->context = (struct context *) sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint) forkret;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void) {

    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
    if ((p->pgdir = setupkvm()) == 0)
        panic("user init: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;

    sz = proc->sz;
    if (n > 0) {
        if ((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    proc->sz = sz;
    switchuvm(proc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void) {
    int i, pid;
    struct proc *np;

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process st  ate from p.
    if ((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = proc->sz;
    np->parent = proc;
    *np->tf = *proc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for (i = 0; i < NOFILE; i++)
        if (proc->ofile[i])
            np->ofile[i] = filedup(proc->ofile[i]);
    np->cwd = idup(proc->cwd);

    safestrcpy(np->name, proc->name, sizeof(proc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void) {
    struct proc *p;
    int fd;

    if (proc == initproc)
        panic("init exiting");

    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (proc->ofile[fd]) {
            fileclose(proc->ofile[fd]);
            proc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(proc->cwd);
    end_op();
    proc->cwd = 0;

    acquire(&ptable.lock);
    proc->parent->childRTime[proc->parent->allChildSize] = proc->rtime;//Saves it
    proc->parent->childCTime[proc->parent->allChildSize] = proc->ctime;//Saves it
    proc->parent->childETime[proc->parent->allChildSize] = ticks;//Saves it
    proc->parent->childPiority[proc->parent->allChildSize] = proc->piority;
    proc->parent->allChildSize++;//Adds it as one dead child
//    cprintf("Real end time is:%d\n",ticks);
//    cprintf("************process end time is:%d\n",proc->parent->childETime[proc->parent->allChildSize-1]);
//    cprintf("(((((((((((process start time is:%d\n",proc->parent->childCTime[proc->parent->allChildSize-1]);
//    cprintf("************process end time is:%d\n",&proc->parent->allChild[proc->parent->allChildSize]->etime);
//    cprintf("(((((((((((process start time is:%d\n",&proc->parent->allChild[proc->parent->allChildSize]->ctime);
    // Parent might be sleeping in wait().
    wakeup1(proc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == proc) {
            p->parent = initproc;
            if (p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.
    proc->state = ZOMBIE;
    proc->etime = ticks;
    sched();
    panic("zombie exit\n");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void) {
    struct proc *p;
    int havekids, pid;

    acquire(&ptable.lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != proc)
                continue;
            havekids = 1;
            if (p->state == ZOMBIE) {
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || proc->killed) {
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(proc, &ptable.lock);  //DOC: wait-sleep
    }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
int GRTExists = 0;
int FRRExists = 0;

void checkForHigherPiority() {
    struct proc *p;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == RUNNABLE || p->state == RUNNING) {
            if (p->piority == HIGH_PIORITY) {
                GRTExists = 1;
            } else if (p->piority == MEDIUM_PIORITY) {
                FRRExists = 1;
            }
        }
    }
}

void
scheduler(void) {
    struct proc *p;
    printRunningProcIsValid = 0;
    while (1) {
        // Enable interrupts on this processor.
        sti();
        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        GRTExists = 0;
        FRRExists = 0;
        checkForHigherPiority();

        //MYCODE
        if (policyChooser == GRT || policyChooser == Q3) {
            int i = 0;
            int bestIndex = -1;
            float bestProcValue = -1;
            for (i = 0; i < NPROC; i++) {
                if (ptable.proc[i].state != RUNNABLE) {
                    continue;
                }
                if (policyChooser == Q3) {
                    if (ptable.proc[i].piority != HIGH_PIORITY) {
                        continue;
                    }
//                    else {
//                        GRTExists = 1;
//                    }
                }
                float down = (float) (ticks - ptable.proc[i].ctime);
                float procValue = 0.0;
                if (down != 0) {
                    procValue = ((float) ptable.proc[i].rtime) / down;
                }
                if (bestProcValue == -1 || bestProcValue > procValue) {
                    bestProcValue = procValue;
                    bestIndex = i;
                }
            }
            if (bestIndex != -1) {
//                cprintf("\nHere:I am %d\n", bestIndex);
                proc = &ptable.proc[bestIndex];
                switchuvm(&ptable.proc[bestIndex]);
//                cprintf("\nHere:I am2\n");
                ptable.proc[bestIndex].processCounter = 0;
//                cprintf("\nHere:I am3\n");
                ptable.proc[bestIndex].state = RUNNING;
//                cprintf("\nHere:I am4\n");
                swtch(&cpu->scheduler, ptable.proc[bestIndex].context);
//                cprintf("\nHere:I am5\n");
                switchkvm();
                printAllRunningProcesses();
//                cprintf("\nHere:I am6\n");
                // Process is done running for now.
                // It should have changed its p->state before coming back.
                proc = 0;
            }
//        }
        }
        if (policyChooser == FRR || (policyChooser == Q3 && GRTExists == 0)) {
            for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
                if (p->state != RUNNABLE) {
                    continue;
                }
                if (policyChooser == Q3) {
                    if (p->piority == MEDIUM_PIORITY) {
                        FRRExists = 1;
                    } else {
//                        if(p->piority == LOW_PIORITY){
//                            cprintf("ERROR:Found a lost piority\n");
//                        }
                        continue;
                    }
                }
                // Switch to chosen process.  It is the process's job
                // to release ptable.lock and then reacquire it
                // before jumping back to us.
                proc = p;
                switchuvm(p);
                p->processCounter = 0;
                p->state = RUNNING;

                swtch(&cpu->scheduler, p->context);
                switchkvm();
                printAllRunningProcesses();
                // Process is done running for now.
                // It should have changed its p->state before coming back.
                proc = 0;
                if(policyChooser==Q3) {
                    checkForHigherPiority();
                    if(GRTExists==1) {
                        break;
                    }
                }
            }
        }
        if (policyChooser == RR || (policyChooser == Q3 &&
                                    GRTExists == 0 && FRRExists == 0)) {
            for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
                if (p->state != RUNNABLE) {
                    continue;
                }
                if (p->piority != LOW_PIORITY) {
                    continue;
//                    cprintf("ERROR:Found a lost piority\n");
                }
                // Switch to chosen process.  It is the process's job
                // to release ptable.lock and then reacquire it
                // before jumping back to us.
                proc = p;
                switchuvm(p);
                p->processCounter = 0;
                p->state = RUNNING;
                swtch(&cpu->scheduler, p->context);
                switchkvm();
                printAllRunningProcesses();
                // Process is done running for now.
                // It should have changed its p->state before coming back.
                proc = 0;
                if(policyChooser==Q3) {
                    checkForHigherPiority();
                    if(GRTExists==1 || FRRExists==1) {
                        break;
                    }
                }
            }
        }
        release(&ptable.lock);
    }
}


// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) {
    int intena;

    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    if (cpu->ncli != 1)
        panic("sched locks");
    if (proc->state == RUNNING)
        panic("sched running");
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    intena = cpu->intena;
    swtch(&proc->context, cpu->scheduler);
    cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    if (policyChooser != GRT) {
        if (proc->piority == HIGH_PIORITY && policyChooser == Q3) {
            return;
        }
        if (proc->processCounter < QUANTA) {
            proc->processCounter++;
            // cprintf("one QUANTA passed!%d\n",proc->pid);
        } else {
            proc->processCounter = 0;
            acquire(&ptable.lock);  //DOC: yieldlock
            proc->state = RUNNABLE;
            sched();
            release(&ptable.lock);
        }
    }
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void) {
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened. 
void
sleep(void *chan, struct spinlock *lk) {
    if (proc == 0)
        panic("sleep");

    if (lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if (lk != &ptable.lock) {  //DOC: sleeplock0
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
    if (lk != &ptable.lock) {  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == SLEEPING && p->chan == chan)
            p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan) {
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid) {
    struct proc *p;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
            p->killed = 1;
            // Wake process from sleep if necessary.
            if (p->state == SLEEPING)
                p->state = RUNNABLE;
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void) {
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

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if (p->state == SLEEPING) {
            getcallerpcs((uint *) p->context->ebp + 2, pc);
            for (i = 0; i < 10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}

void sortProcessFIFO() {
    int i = 0;
    for (i = 0; i < NPROC; i++) {
        int j = 0;
        if (ptable.proc[i].state != RUNNABLE) {
            continue;
        }
        int minIndex = i;
        for (j = i + 1; j < NPROC; ++j) {
            if (ptable.proc[j].state != RUNNABLE) {
                continue;
            }
            if (ptable.proc[j].ctime < ptable.proc[i].ctime) {
                minIndex = j;
            }
        }
        struct proc temp;
        temp = ptable.proc[i];
        ptable.proc[i] = ptable.proc[minIndex];
        ptable.proc[minIndex] = temp;
    }
//    cprintf("--------------------------\n");
//    for (i = 0; i < NPROC; ++i) {
//        if(ptable.proc[i].state==RUNNABLE||ptable.proc[i].state==RUNNING) {
//            cprintf("Proc%d:%d\n", i, ptable.proc[i].ctime);
//        }
//    }
//    cprintf("**************************\n");
}

void printAllRunningProcesses() {
    if (printRunningProcIsValid == 1) {
        cprintf("\n$$$$$$$$Running Processes are:");
        int i = 0;
        for (i = 0; i < NPROC; ++i) {
//            if (ptable.proc[i].state == RUNNABLE ||
//                ptable.proc[i].state == RUNNING) {
            if (ptable.proc[i].pid != 0) {
                cprintf("<%d>", ptable.proc[i].pid);
            }
        }
        cprintf("\n");
    }
}

