8800 # Initial process execs /init.
8801 # This code runs in user space.
8802 
8803 #include "syscall.h"
8804 #include "traps.h"
8805 
8806 
8807 # exec(init, argv)
8808 .globl start
8809 start:
8810   pushl $argv
8811   pushl $init
8812   pushl $0  // where caller pc would be
8813   movl $SYS_exec, %eax
8814   int $T_SYSCALL
8815 
8816 # for(;;) exit();
8817 exit:
8818   movl $SYS_exit, %eax
8819   int $T_SYSCALL
8820   jmp exit
8821 
8822 # char init[] = "/init\0";
8823 init:
8824   .string "/init\0"
8825 
8826 # char *argv[] = { init, 0 };
8827 .p2align 2
8828 argv:
8829   .long init
8830   .long 0
8831 
8832 
8833 
8834 
8835 
8836 
8837 
8838 
8839 
8840 
8841 
8842 
8843 
8844 
8845 
8846 
8847 
8848 
8849 
