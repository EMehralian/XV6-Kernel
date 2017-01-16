8850 #include "syscall.h"
8851 #include "traps.h"
8852 
8853 #define SYSCALL(name) \
8854   .globl name; \
8855   name: \
8856     movl $SYS_ ## name, %eax; \
8857     int $T_SYSCALL; \
8858     ret
8859 
8860 SYSCALL(fork)
8861 SYSCALL(exit)
8862 SYSCALL(wait)
8863 SYSCALL(pipe)
8864 SYSCALL(read)
8865 SYSCALL(write)
8866 SYSCALL(close)
8867 SYSCALL(kill)
8868 SYSCALL(exec)
8869 SYSCALL(open)
8870 SYSCALL(mknod)
8871 SYSCALL(unlink)
8872 SYSCALL(fstat)
8873 SYSCALL(link)
8874 SYSCALL(mkdir)
8875 SYSCALL(chdir)
8876 SYSCALL(dup)
8877 SYSCALL(getpid)
8878 SYSCALL(sbrk)
8879 SYSCALL(sleep)
8880 SYSCALL(uptime)
8881 SYSCALL(getppid)
8882 SYSCALL(getPerformanceData)
8883 
8884 
8885 
8886 
8887 
8888 
8889 
8890 
8891 
8892 
8893 
8894 
8895 
8896 
8897 
8898 
8899 
