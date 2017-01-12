9600 // Boot loader.
9601 //
9602 // Part of the boot block, along with bootasm.S, which calls bootmain().
9603 // bootasm.S has put the processor into protected 32-bit mode.
9604 // bootmain() loads an ELF kernel image from the disk starting at
9605 // sector 1 and then jumps to the kernel entry routine.
9606 
9607 #include "types.h"
9608 #include "elf.h"
9609 #include "x86.h"
9610 #include "memlayout.h"
9611 
9612 #define SECTSIZE  512
9613 
9614 void readseg(uchar*, uint, uint);
9615 
9616 void
9617 bootmain(void)
9618 {
9619   struct elfhdr *elf;
9620   struct proghdr *ph, *eph;
9621   void (*entry)(void);
9622   uchar* pa;
9623 
9624   elf = (struct elfhdr*)0x10000;  // scratch space
9625 
9626   // Read 1st page off disk
9627   readseg((uchar*)elf, 4096, 0);
9628 
9629   // Is this an ELF executable?
9630   if(elf->magic != ELF_MAGIC)
9631     return;  // let bootasm.S handle error
9632 
9633   // Load each program segment (ignores ph flags).
9634   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
9635   eph = ph + elf->phnum;
9636   for(; ph < eph; ph++){
9637     pa = (uchar*)ph->paddr;
9638     readseg(pa, ph->filesz, ph->off);
9639     if(ph->memsz > ph->filesz)
9640       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
9641   }
9642 
9643   // Call the entry point from the ELF header.
9644   // Does not return!
9645   entry = (void(*)(void))(elf->entry);
9646   entry();
9647 }
9648 
9649 
9650 void
9651 waitdisk(void)
9652 {
9653   // Wait for disk ready.
9654   while((inb(0x1F7) & 0xC0) != 0x40)
9655     ;
9656 }
9657 
9658 // Read a single sector at offset into dst.
9659 void
9660 readsect(void *dst, uint offset)
9661 {
9662   // Issue command.
9663   waitdisk();
9664   outb(0x1F2, 1);   // count = 1
9665   outb(0x1F3, offset);
9666   outb(0x1F4, offset >> 8);
9667   outb(0x1F5, offset >> 16);
9668   outb(0x1F6, (offset >> 24) | 0xE0);
9669   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
9670 
9671   // Read data.
9672   waitdisk();
9673   insl(0x1F0, dst, SECTSIZE/4);
9674 }
9675 
9676 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
9677 // Might copy more than asked.
9678 void
9679 readseg(uchar* pa, uint count, uint offset)
9680 {
9681   uchar* epa;
9682 
9683   epa = pa + count;
9684 
9685   // Round down to sector boundary.
9686   pa -= offset % SECTSIZE;
9687 
9688   // Translate from bytes to sectors; kernel starts at sector 1.
9689   offset = (offset / SECTSIZE) + 1;
9690 
9691   // If this is too slow, we could read lots of sectors at a time.
9692   // We'd write more to memory than asked, but it doesn't matter --
9693   // we load in increasing order.
9694   for(; pa < epa; pa += SECTSIZE, offset++)
9695     readsect(pa, offset);
9696 }
9697 
9698 
9699 
