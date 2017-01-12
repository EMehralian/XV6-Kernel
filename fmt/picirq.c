7900 // Intel 8259A programmable interrupt controllers.
7901 
7902 #include "types.h"
7903 #include "x86.h"
7904 #include "traps.h"
7905 
7906 // I/O Addresses of the two programmable interrupt controllers
7907 #define IO_PIC1         0x20    // Master (IRQs 0-7)
7908 #define IO_PIC2         0xA0    // Slave (IRQs 8-15)
7909 
7910 #define IRQ_SLAVE       2       // IRQ at which slave connects to master
7911 
7912 // Current IRQ mask.
7913 // Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
7914 static ushort irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);
7915 
7916 static void
7917 picsetmask(ushort mask)
7918 {
7919   irqmask = mask;
7920   outb(IO_PIC1+1, mask);
7921   outb(IO_PIC2+1, mask >> 8);
7922 }
7923 
7924 void
7925 picenable(int irq)
7926 {
7927   picsetmask(irqmask & ~(1<<irq));
7928 }
7929 
7930 // Initialize the 8259A interrupt controllers.
7931 void
7932 picinit(void)
7933 {
7934   // mask all interrupts
7935   outb(IO_PIC1+1, 0xFF);
7936   outb(IO_PIC2+1, 0xFF);
7937 
7938   // Set up master (8259A-1)
7939 
7940   // ICW1:  0001g0hi
7941   //    g:  0 = edge triggering, 1 = level triggering
7942   //    h:  0 = cascaded PICs, 1 = master only
7943   //    i:  0 = no ICW4, 1 = ICW4 required
7944   outb(IO_PIC1, 0x11);
7945 
7946   // ICW2:  Vector offset
7947   outb(IO_PIC1+1, T_IRQ0);
7948 
7949 
7950   // ICW3:  (master PIC) bit mask of IR lines connected to slaves
7951   //        (slave PIC) 3-bit # of slave's connection to master
7952   outb(IO_PIC1+1, 1<<IRQ_SLAVE);
7953 
7954   // ICW4:  000nbmap
7955   //    n:  1 = special fully nested mode
7956   //    b:  1 = buffered mode
7957   //    m:  0 = slave PIC, 1 = master PIC
7958   //      (ignored when b is 0, as the master/slave role
7959   //      can be hardwired).
7960   //    a:  1 = Automatic EOI mode
7961   //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
7962   outb(IO_PIC1+1, 0x3);
7963 
7964   // Set up slave (8259A-2)
7965   outb(IO_PIC2, 0x11);                  // ICW1
7966   outb(IO_PIC2+1, T_IRQ0 + 8);      // ICW2
7967   outb(IO_PIC2+1, IRQ_SLAVE);           // ICW3
7968   // NB Automatic EOI mode doesn't tend to work on the slave.
7969   // Linux source code says it's "to be investigated".
7970   outb(IO_PIC2+1, 0x3);                 // ICW4
7971 
7972   // OCW3:  0ef01prs
7973   //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
7974   //    p:  0 = no polling, 1 = polling mode
7975   //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
7976   outb(IO_PIC1, 0x68);             // clear specific mask
7977   outb(IO_PIC1, 0x0a);             // read IRR by default
7978 
7979   outb(IO_PIC2, 0x68);             // OCW3
7980   outb(IO_PIC2, 0x0a);             // OCW3
7981 
7982   if(irqmask != 0xFFFF)
7983     picsetmask(irqmask);
7984 }
7985 
7986 
7987 
7988 
7989 
7990 
7991 
7992 
7993 
7994 
7995 
7996 
7997 
7998 
7999 
8000 // Blank page.
8001 
8002 
8003 
8004 
8005 
8006 
8007 
8008 
8009 
8010 
8011 
8012 
8013 
8014 
8015 
8016 
8017 
8018 
8019 
8020 
8021 
8022 
8023 
8024 
8025 
8026 
8027 
8028 
8029 
8030 
8031 
8032 
8033 
8034 
8035 
8036 
8037 
8038 
8039 
8040 
8041 
8042 
8043 
8044 
8045 
8046 
8047 
8048 
8049 
