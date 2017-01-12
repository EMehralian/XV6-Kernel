9500 #include "asm.h"
9501 #include "memlayout.h"
9502 #include "mmu.h"
9503 
9504 # Start the first CPU: switch to 32-bit protected mode, jump into C.
9505 # The BIOS loads this code from the first sector of the hard disk into
9506 # memory at physical address 0x7c00 and starts executing in real mode
9507 # with %cs=0 %ip=7c00.
9508 
9509 .code16                       # Assemble for 16-bit mode
9510 .globl start
9511 start:
9512   cli                         # BIOS enabled interrupts; disable
9513 
9514   # Zero data segment registers DS, ES, and SS.
9515   xorw    %ax,%ax             # Set %ax to zero
9516   movw    %ax,%ds             # -> Data Segment
9517   movw    %ax,%es             # -> Extra Segment
9518   movw    %ax,%ss             # -> Stack Segment
9519 
9520   # Physical address line A20 is tied to zero so that the first PCs
9521   # with 2 MB would run software that assumed 1 MB.  Undo that.
9522 seta20.1:
9523   inb     $0x64,%al               # Wait for not busy
9524   testb   $0x2,%al
9525   jnz     seta20.1
9526 
9527   movb    $0xd1,%al               # 0xd1 -> port 0x64
9528   outb    %al,$0x64
9529 
9530 seta20.2:
9531   inb     $0x64,%al               # Wait for not busy
9532   testb   $0x2,%al
9533   jnz     seta20.2
9534 
9535   movb    $0xdf,%al               # 0xdf -> port 0x60
9536   outb    %al,$0x60
9537 
9538   # Switch from real to protected mode.  Use a bootstrap GDT that makes
9539   # virtual addresses map directly to physical addresses so that the
9540   # effective memory map doesn't change during the transition.
9541   lgdt    gdtdesc
9542   movl    %cr0, %eax
9543   orl     $CR0_PE, %eax
9544   movl    %eax, %cr0
9545 
9546 
9547 
9548 
9549 
9550   # Complete the transition to 32-bit protected mode by using a long jmp
9551   # to reload %cs and %eip.  The segment descriptors are set up with no
9552   # translation, so that the mapping is still the identity mapping.
9553   ljmp    $(SEG_KCODE<<3), $start32
9554 
9555 .code32  # Tell assembler to generate 32-bit code now.
9556 start32:
9557   # Set up the protected-mode data segment registers
9558   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
9559   movw    %ax, %ds                # -> DS: Data Segment
9560   movw    %ax, %es                # -> ES: Extra Segment
9561   movw    %ax, %ss                # -> SS: Stack Segment
9562   movw    $0, %ax                 # Zero segments not ready for use
9563   movw    %ax, %fs                # -> FS
9564   movw    %ax, %gs                # -> GS
9565 
9566   # Set up the stack pointer and call into C.
9567   movl    $start, %esp
9568   call    bootmain
9569 
9570   # If bootmain returns (it shouldn't), trigger a Bochs
9571   # breakpoint if running under Bochs, then loop.
9572   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
9573   movw    %ax, %dx
9574   outw    %ax, %dx
9575   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
9576   outw    %ax, %dx
9577 spin:
9578   jmp     spin
9579 
9580 # Bootstrap GDT
9581 .p2align 2                                # force 4 byte alignment
9582 gdt:
9583   SEG_NULLASM                             # null seg
9584   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
9585   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
9586 
9587 gdtdesc:
9588   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
9589   .long   gdt                             # address gdt
9590 
9591 
9592 
9593 
9594 
9595 
9596 
9597 
9598 
9599 
