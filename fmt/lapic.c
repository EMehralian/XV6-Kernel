7450 // The local APIC manages internal (non-I/O) interrupts.
7451 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
7452 
7453 #include "param.h"
7454 #include "types.h"
7455 #include "defs.h"
7456 #include "date.h"
7457 #include "memlayout.h"
7458 #include "traps.h"
7459 #include "mmu.h"
7460 #include "x86.h"
7461 #include "proc.h"  // ncpu
7462 
7463 // Local APIC registers, divided by 4 for use as uint[] indices.
7464 #define ID      (0x0020/4)   // ID
7465 #define VER     (0x0030/4)   // Version
7466 #define TPR     (0x0080/4)   // Task Priority
7467 #define EOI     (0x00B0/4)   // EOI
7468 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
7469   #define ENABLE     0x00000100   // Unit Enable
7470 #define ESR     (0x0280/4)   // Error Status
7471 #define ICRLO   (0x0300/4)   // Interrupt Command
7472   #define INIT       0x00000500   // INIT/RESET
7473   #define STARTUP    0x00000600   // Startup IPI
7474   #define DELIVS     0x00001000   // Delivery status
7475   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
7476   #define DEASSERT   0x00000000
7477   #define LEVEL      0x00008000   // Level triggered
7478   #define BCAST      0x00080000   // Send to all APICs, including self.
7479   #define BUSY       0x00001000
7480   #define FIXED      0x00000000
7481 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
7482 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
7483   #define X1         0x0000000B   // divide counts by 1
7484   #define PERIODIC   0x00020000   // Periodic
7485 #define PCINT   (0x0340/4)   // Performance Counter LVT
7486 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
7487 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
7488 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
7489   #define MASKED     0x00010000   // Interrupt masked
7490 #define TICR    (0x0380/4)   // Timer Initial Count
7491 #define TCCR    (0x0390/4)   // Timer Current Count
7492 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
7493 
7494 volatile uint *lapic;  // Initialized in mp.c
7495 
7496 
7497 
7498 
7499 
7500 static void
7501 lapicw(int index, int value)
7502 {
7503   lapic[index] = value;
7504   lapic[ID];  // wait for write to finish, by reading
7505 }
7506 
7507 
7508 
7509 
7510 
7511 
7512 
7513 
7514 
7515 
7516 
7517 
7518 
7519 
7520 
7521 
7522 
7523 
7524 
7525 
7526 
7527 
7528 
7529 
7530 
7531 
7532 
7533 
7534 
7535 
7536 
7537 
7538 
7539 
7540 
7541 
7542 
7543 
7544 
7545 
7546 
7547 
7548 
7549 
7550 void
7551 lapicinit(void)
7552 {
7553   if(!lapic)
7554     return;
7555 
7556   // Enable local APIC; set spurious interrupt vector.
7557   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
7558 
7559   // The timer repeatedly counts down at bus frequency
7560   // from lapic[TICR] and then issues an interrupt.
7561   // If xv6 cared more about precise timekeeping,
7562   // TICR would be calibrated using an external time source.
7563   lapicw(TDCR, X1);
7564   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
7565   lapicw(TICR, 10000000);
7566 
7567   // Disable logical interrupt lines.
7568   lapicw(LINT0, MASKED);
7569   lapicw(LINT1, MASKED);
7570 
7571   // Disable performance counter overflow interrupts
7572   // on machines that provide that interrupt entry.
7573   if(((lapic[VER]>>16) & 0xFF) >= 4)
7574     lapicw(PCINT, MASKED);
7575 
7576   // Map error interrupt to IRQ_ERROR.
7577   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
7578 
7579   // Clear error status register (requires back-to-back writes).
7580   lapicw(ESR, 0);
7581   lapicw(ESR, 0);
7582 
7583   // Ack any outstanding interrupts.
7584   lapicw(EOI, 0);
7585 
7586   // Send an Init Level De-Assert to synchronise arbitration ID's.
7587   lapicw(ICRHI, 0);
7588   lapicw(ICRLO, BCAST | INIT | LEVEL);
7589   while(lapic[ICRLO] & DELIVS)
7590     ;
7591 
7592   // Enable interrupts on the APIC (but not on the processor).
7593   lapicw(TPR, 0);
7594 }
7595 
7596 
7597 
7598 
7599 
7600 int
7601 cpunum(void)
7602 {
7603   int apicid, i;
7604 
7605   // Cannot call cpu when interrupts are enabled:
7606   // result not guaranteed to last long enough to be used!
7607   // Would prefer to panic but even printing is chancy here:
7608   // almost everything, including cprintf and panic, calls cpu,
7609   // often indirectly through acquire and release.
7610   if(readeflags()&FL_IF){
7611     static int n;
7612     if(n++ == 0)
7613       cprintf("cpu called from %x with interrupts enabled\n",
7614         __builtin_return_address(0));
7615   }
7616 
7617   if (!lapic)
7618     return 0;
7619 
7620   apicid = lapic[ID] >> 24;
7621   for (i = 0; i < ncpu; ++i) {
7622     if (cpus[i].apicid == apicid)
7623       return i;
7624   }
7625   panic("unknown apicid\n");
7626 }
7627 
7628 // Acknowledge interrupt.
7629 void
7630 lapiceoi(void)
7631 {
7632   if(lapic)
7633     lapicw(EOI, 0);
7634 }
7635 
7636 // Spin for a given number of microseconds.
7637 // On real hardware would want to tune this dynamically.
7638 void
7639 microdelay(int us)
7640 {
7641 }
7642 
7643 
7644 
7645 
7646 
7647 
7648 
7649 
7650 #define CMOS_PORT    0x70
7651 #define CMOS_RETURN  0x71
7652 
7653 // Start additional processor running entry code at addr.
7654 // See Appendix B of MultiProcessor Specification.
7655 void
7656 lapicstartap(uchar apicid, uint addr)
7657 {
7658   int i;
7659   ushort *wrv;
7660 
7661   // "The BSP must initialize CMOS shutdown code to 0AH
7662   // and the warm reset vector (DWORD based at 40:67) to point at
7663   // the AP startup code prior to the [universal startup algorithm]."
7664   outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
7665   outb(CMOS_PORT+1, 0x0A);
7666   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
7667   wrv[0] = 0;
7668   wrv[1] = addr >> 4;
7669 
7670   // "Universal startup algorithm."
7671   // Send INIT (level-triggered) interrupt to reset other CPU.
7672   lapicw(ICRHI, apicid<<24);
7673   lapicw(ICRLO, INIT | LEVEL | ASSERT);
7674   microdelay(200);
7675   lapicw(ICRLO, INIT | LEVEL);
7676   microdelay(100);    // should be 10ms, but too slow in Bochs!
7677 
7678   // Send startup IPI (twice!) to enter code.
7679   // Regular hardware is supposed to only accept a STARTUP
7680   // when it is in the halted state due to an INIT.  So the second
7681   // should be ignored, but it is part of the official Intel algorithm.
7682   // Bochs complains about the second one.  Too bad for Bochs.
7683   for(i = 0; i < 2; i++){
7684     lapicw(ICRHI, apicid<<24);
7685     lapicw(ICRLO, STARTUP | (addr>>12));
7686     microdelay(200);
7687   }
7688 }
7689 
7690 
7691 
7692 
7693 
7694 
7695 
7696 
7697 
7698 
7699 
7700 #define CMOS_STATA   0x0a
7701 #define CMOS_STATB   0x0b
7702 #define CMOS_UIP    (1 << 7)        // RTC update in progress
7703 
7704 #define SECS    0x00
7705 #define MINS    0x02
7706 #define HOURS   0x04
7707 #define DAY     0x07
7708 #define MONTH   0x08
7709 #define YEAR    0x09
7710 
7711 static uint cmos_read(uint reg)
7712 {
7713   outb(CMOS_PORT,  reg);
7714   microdelay(200);
7715 
7716   return inb(CMOS_RETURN);
7717 }
7718 
7719 static void fill_rtcdate(struct rtcdate *r)
7720 {
7721   r->second = cmos_read(SECS);
7722   r->minute = cmos_read(MINS);
7723   r->hour   = cmos_read(HOURS);
7724   r->day    = cmos_read(DAY);
7725   r->month  = cmos_read(MONTH);
7726   r->year   = cmos_read(YEAR);
7727 }
7728 
7729 // qemu seems to use 24-hour GWT and the values are BCD encoded
7730 void cmostime(struct rtcdate *r)
7731 {
7732   struct rtcdate t1, t2;
7733   int sb, bcd;
7734 
7735   sb = cmos_read(CMOS_STATB);
7736 
7737   bcd = (sb & (1 << 2)) == 0;
7738 
7739   // make sure CMOS doesn't modify time while we read it
7740   for(;;) {
7741     fill_rtcdate(&t1);
7742     if(cmos_read(CMOS_STATA) & CMOS_UIP)
7743         continue;
7744     fill_rtcdate(&t2);
7745     if(memcmp(&t1, &t2, sizeof(t1)) == 0)
7746       break;
7747   }
7748 
7749 
7750   // convert
7751   if(bcd) {
7752 #define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
7753     CONV(second);
7754     CONV(minute);
7755     CONV(hour  );
7756     CONV(day   );
7757     CONV(month );
7758     CONV(year  );
7759 #undef     CONV
7760   }
7761 
7762   *r = t1;
7763   r->year += 2000;
7764 }
7765 
7766 
7767 
7768 
7769 
7770 
7771 
7772 
7773 
7774 
7775 
7776 
7777 
7778 
7779 
7780 
7781 
7782 
7783 
7784 
7785 
7786 
7787 
7788 
7789 
7790 
7791 
7792 
7793 
7794 
7795 
7796 
7797 
7798 
7799 
