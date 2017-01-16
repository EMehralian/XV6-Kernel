3350 #include "types.h"
3351 #include "defs.h"
3352 #include "param.h"
3353 #include "memlayout.h"
3354 #include "mmu.h"
3355 #include "proc.h"
3356 #include "x86.h"
3357 #include "traps.h"
3358 #include "spinlock.h"
3359 
3360 // Interrupt descriptor table (shared by all CPUs).
3361 struct gatedesc idt[256];
3362 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3363 struct spinlock tickslock;
3364 uint ticks;
3365 
3366 void
3367 tvinit(void)
3368 {
3369   int i;
3370 
3371   for(i = 0; i < 256; i++)
3372     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3373   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3374 
3375   initlock(&tickslock, "time");
3376 }
3377 
3378 void
3379 idtinit(void)
3380 {
3381   lidt(idt, sizeof(idt));
3382 }
3383 
3384 
3385 
3386 
3387 
3388 
3389 
3390 
3391 
3392 
3393 
3394 
3395 
3396 
3397 
3398 
3399 
3400 void
3401 trap(struct trapframe *tf)
3402 {
3403   if(tf->trapno == T_SYSCALL){
3404     if(proc->killed)
3405       exit();
3406     proc->tf = tf;
3407     syscall();
3408     if(proc->killed)
3409       exit();
3410     return;
3411   }
3412 
3413   switch(tf->trapno){
3414   case T_IRQ0 + IRQ_TIMER:
3415     if(cpunum() == 0){
3416       acquire(&tickslock);
3417       ticks++;
3418       if(proc && proc->state == RUNNING)
3419         {
3420           proc->rtime++;
3421           // cprintf("ticked\n\n\n");
3422         }
3423       wakeup(&ticks);
3424       release(&tickslock);
3425     }
3426     lapiceoi();
3427     break;
3428   case T_IRQ0 + IRQ_IDE:
3429     ideintr();
3430     lapiceoi();
3431     break;
3432   case T_IRQ0 + IRQ_IDE+1:
3433     // Bochs generates spurious IDE1 interrupts.
3434     break;
3435   case T_IRQ0 + IRQ_KBD:
3436     kbdintr();
3437     lapiceoi();
3438     break;
3439   case T_IRQ0 + IRQ_COM1:
3440     uartintr();
3441     lapiceoi();
3442     break;
3443   case T_IRQ0 + 7:
3444   case T_IRQ0 + IRQ_SPURIOUS:
3445     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3446             cpunum(), tf->cs, tf->eip);
3447     lapiceoi();
3448     break;
3449 
3450 
3451   default:
3452     if(proc == 0 || (tf->cs&3) == 0){
3453       // In kernel, it must be our mistake.
3454       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3455               tf->trapno, cpunum(), tf->eip, rcr2());
3456       panic("trap");
3457     }
3458     // In user space, assume process misbehaved.
3459     cprintf("pid %d %s: trap %d err %d on cpu %d "
3460             "eip 0x%x addr 0x%x--kill proc\n",
3461             proc->pid, proc->name, tf->trapno, tf->err, cpunum(), tf->eip,
3462             rcr2());
3463     proc->killed = 1;
3464   }
3465 
3466   // Force process exit if it has been killed and is in user space.
3467   // (If it is still executing in the kernel, let it keep running
3468   // until it gets to the regular system call return.)
3469   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
3470     exit();
3471 
3472   // Force process to give up CPU on clock tick.
3473   // If interrupts were on while locks held, would need to check nlock.
3474   if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
3475     yield();
3476 
3477   // Check if the process has been killed since we yielded
3478   if(proc && proc->killed && (tf->cs&3) == DPL_USER)
3479     exit();
3480 }
3481 
3482 
3483 
3484 
3485 
3486 
3487 
3488 
3489 
3490 
3491 
3492 
3493 
3494 
3495 
3496 
3497 
3498 
3499 
