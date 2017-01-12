7800 // The I/O APIC manages hardware interrupts for an SMP system.
7801 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
7802 // See also picirq.c.
7803 
7804 #include "types.h"
7805 #include "defs.h"
7806 #include "traps.h"
7807 
7808 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
7809 
7810 #define REG_ID     0x00  // Register index: ID
7811 #define REG_VER    0x01  // Register index: version
7812 #define REG_TABLE  0x10  // Redirection table base
7813 
7814 // The redirection table starts at REG_TABLE and uses
7815 // two registers to configure each interrupt.
7816 // The first (low) register in a pair contains configuration bits.
7817 // The second (high) register contains a bitmask telling which
7818 // CPUs can serve that interrupt.
7819 #define INT_DISABLED   0x00010000  // Interrupt disabled
7820 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
7821 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
7822 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
7823 
7824 volatile struct ioapic *ioapic;
7825 
7826 // IO APIC MMIO structure: write reg, then read or write data.
7827 struct ioapic {
7828   uint reg;
7829   uint pad[3];
7830   uint data;
7831 };
7832 
7833 static uint
7834 ioapicread(int reg)
7835 {
7836   ioapic->reg = reg;
7837   return ioapic->data;
7838 }
7839 
7840 static void
7841 ioapicwrite(int reg, uint data)
7842 {
7843   ioapic->reg = reg;
7844   ioapic->data = data;
7845 }
7846 
7847 
7848 
7849 
7850 void
7851 ioapicinit(void)
7852 {
7853   int i, id, maxintr;
7854 
7855   if(!ismp)
7856     return;
7857 
7858   ioapic = (volatile struct ioapic*)IOAPIC;
7859   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
7860   id = ioapicread(REG_ID) >> 24;
7861   if(id != ioapicid)
7862     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
7863 
7864   // Mark all interrupts edge-triggered, active high, disabled,
7865   // and not routed to any CPUs.
7866   for(i = 0; i <= maxintr; i++){
7867     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
7868     ioapicwrite(REG_TABLE+2*i+1, 0);
7869   }
7870 }
7871 
7872 void
7873 ioapicenable(int irq, int cpunum)
7874 {
7875   if(!ismp)
7876     return;
7877 
7878   // Mark interrupt edge-triggered, active high,
7879   // enabled, and routed to the given cpunum,
7880   // which happens to be that cpu's APIC ID.
7881   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
7882   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
7883 }
7884 
7885 
7886 
7887 
7888 
7889 
7890 
7891 
7892 
7893 
7894 
7895 
7896 
7897 
7898 
7899 
