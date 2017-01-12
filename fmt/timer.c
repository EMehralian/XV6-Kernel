8650 // Intel 8253/8254/82C54 Programmable Interval Timer (PIT).
8651 // Only used on uniprocessors;
8652 // SMP machines use the local APIC timer.
8653 
8654 #include "types.h"
8655 #include "defs.h"
8656 #include "traps.h"
8657 #include "x86.h"
8658 
8659 #define IO_TIMER1       0x040           // 8253 Timer #1
8660 
8661 // Frequency of all three count-down timers;
8662 // (TIMER_FREQ/freq) is the appropriate count
8663 // to generate a frequency of freq Hz.
8664 
8665 #define TIMER_FREQ      1193182
8666 #define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))
8667 
8668 #define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
8669 #define TIMER_SEL0      0x00    // select counter 0
8670 #define TIMER_RATEGEN   0x04    // mode 2, rate generator
8671 #define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
8672 
8673 void
8674 timerinit(void)
8675 {
8676   // Interrupt 100 times/sec.
8677   outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
8678   outb(IO_TIMER1, TIMER_DIV(100) % 256);
8679   outb(IO_TIMER1, TIMER_DIV(100) / 256);
8680   picenable(IRQ_TIMER);
8681 }
8682 
8683 
8684 
8685 
8686 
8687 
8688 
8689 
8690 
8691 
8692 
8693 
8694 
8695 
8696 
8697 
8698 
8699 
