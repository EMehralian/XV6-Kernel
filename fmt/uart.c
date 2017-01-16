8700 // Intel 8250 serial port (UART).
8701 
8702 #include "types.h"
8703 #include "defs.h"
8704 #include "param.h"
8705 #include "traps.h"
8706 #include "spinlock.h"
8707 #include "sleeplock.h"
8708 #include "fs.h"
8709 #include "file.h"
8710 #include "mmu.h"
8711 #include "proc.h"
8712 #include "x86.h"
8713 
8714 #define COM1    0x3f8
8715 
8716 static int uart;    // is there a uart?
8717 
8718 void
8719 uartinit(void)
8720 {
8721   char *p;
8722 
8723   // Turn off the FIFO
8724   outb(COM1+2, 0);
8725 
8726   // 9600 baud, 8 data bits, 1 stop bit, parity off.
8727   outb(COM1+3, 0x80);    // Unlock divisor
8728   outb(COM1+0, 115200/9600);
8729   outb(COM1+1, 0);
8730   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
8731   outb(COM1+4, 0);
8732   outb(COM1+1, 0x01);    // Enable receive interrupts.
8733 
8734   // If status is 0xFF, no serial port.
8735   if(inb(COM1+5) == 0xFF)
8736     return;
8737   uart = 1;
8738 
8739   // Acknowledge pre-existing interrupt conditions;
8740   // enable interrupts.
8741   inb(COM1+2);
8742   inb(COM1+0);
8743   picenable(IRQ_COM1);
8744   ioapicenable(IRQ_COM1, 0);
8745 
8746   // Announce that we're here.
8747   for(p="xv6...\n"; *p; p++)
8748     uartputc(*p);
8749 }
8750 void
8751 uartputc(int c)
8752 {
8753   int i;
8754 
8755   if(!uart)
8756     return;
8757   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
8758     microdelay(10);
8759   outb(COM1+0, c);
8760 }
8761 
8762 static int
8763 uartgetc(void)
8764 {
8765   if(!uart)
8766     return -1;
8767   if(!(inb(COM1+5) & 0x01))
8768     return -1;
8769   return inb(COM1+0);
8770 }
8771 
8772 void
8773 uartintr(void)
8774 {
8775   consoleintr(uartgetc);
8776 }
8777 
8778 
8779 
8780 
8781 
8782 
8783 
8784 
8785 
8786 
8787 
8788 
8789 
8790 
8791 
8792 
8793 
8794 
8795 
8796 
8797 
8798 
8799 
