8900 // init: The initial user-level program
8901 
8902 #include "types.h"
8903 #include "stat.h"
8904 #include "user.h"
8905 #include "fcntl.h"
8906 
8907 char *argv[] = { "sh", 0 };
8908 
8909 int
8910 main(void)
8911 {
8912   int pid, wpid;
8913 
8914   if(open("console", O_RDWR) < 0){
8915     mknod("console", 1, 1);
8916     open("console", O_RDWR);
8917   }
8918   dup(0);  // stdout
8919   dup(0);  // stderr
8920 
8921   for(;;){
8922     printf(1, "init: starting sh\n");
8923     pid = fork();
8924     if(pid < 0){
8925       printf(1, "init: fork failed\n");
8926       exit();
8927     }
8928     if(pid == 0){
8929       exec("sh", argv);
8930       printf(1, "init: exec sh failed\n");
8931       exit();
8932     }
8933     while((wpid=wait()) >= 0 && wpid != pid)
8934       printf(1, "zombie!\n");
8935   }
8936 }
8937 
8938 
8939 
8940 
8941 
8942 
8943 
8944 
8945 
8946 
8947 
8948 
8949 
