8950 // Shell.
8951 
8952 #include "types.h"
8953 #include "user.h"
8954 #include "fcntl.h"
8955 
8956 // Parsed command representation
8957 #define EXEC  1
8958 #define REDIR 2
8959 #define PIPE  3
8960 #define LIST  4
8961 #define BACK  5
8962 
8963 #define MAXARGS 10
8964 
8965 struct cmd {
8966   int type;
8967 };
8968 
8969 struct execcmd {
8970   int type;
8971   char *argv[MAXARGS];
8972   char *eargv[MAXARGS];
8973 };
8974 
8975 struct redircmd {
8976   int type;
8977   struct cmd *cmd;
8978   char *file;
8979   char *efile;
8980   int mode;
8981   int fd;
8982 };
8983 
8984 struct pipecmd {
8985   int type;
8986   struct cmd *left;
8987   struct cmd *right;
8988 };
8989 
8990 struct listcmd {
8991   int type;
8992   struct cmd *left;
8993   struct cmd *right;
8994 };
8995 
8996 struct backcmd {
8997   int type;
8998   struct cmd *cmd;
8999 };
9000 int fork1(void);  // Fork but panics on failure.
9001 void panic(char*);
9002 struct cmd *parsecmd(char*);
9003 
9004 // Execute cmd.  Never returns.
9005 void
9006 runcmd(struct cmd *cmd)
9007 {
9008   int p[2];
9009   struct backcmd *bcmd;
9010   struct execcmd *ecmd;
9011   struct listcmd *lcmd;
9012   struct pipecmd *pcmd;
9013   struct redircmd *rcmd;
9014 
9015   if(cmd == 0)
9016     exit();
9017 
9018   switch(cmd->type){
9019   default:
9020     panic("runcmd");
9021 
9022   case EXEC:
9023     ecmd = (struct execcmd*)cmd;
9024     if(ecmd->argv[0] == 0)
9025       exit();
9026     exec(ecmd->argv[0], ecmd->argv);
9027     printf(2, "exec %s failed\n", ecmd->argv[0]);
9028     break;
9029 
9030   case REDIR:
9031     rcmd = (struct redircmd*)cmd;
9032     close(rcmd->fd);
9033     if(open(rcmd->file, rcmd->mode) < 0){
9034       printf(2, "open %s failed\n", rcmd->file);
9035       exit();
9036     }
9037     runcmd(rcmd->cmd);
9038     break;
9039 
9040   case LIST:
9041     lcmd = (struct listcmd*)cmd;
9042     if(fork1() == 0)
9043       runcmd(lcmd->left);
9044     wait();
9045     runcmd(lcmd->right);
9046     break;
9047 
9048 
9049 
9050   case PIPE:
9051     pcmd = (struct pipecmd*)cmd;
9052     if(pipe(p) < 0)
9053       panic("pipe");
9054     if(fork1() == 0){
9055       close(1);
9056       dup(p[1]);
9057       close(p[0]);
9058       close(p[1]);
9059       runcmd(pcmd->left);
9060     }
9061     if(fork1() == 0){
9062       close(0);
9063       dup(p[0]);
9064       close(p[0]);
9065       close(p[1]);
9066       runcmd(pcmd->right);
9067     }
9068     close(p[0]);
9069     close(p[1]);
9070     wait();
9071     wait();
9072     break;
9073 
9074   case BACK:
9075     bcmd = (struct backcmd*)cmd;
9076     if(fork1() == 0)
9077       runcmd(bcmd->cmd);
9078     break;
9079   }
9080   exit();
9081 }
9082 
9083 int
9084 getcmd(char *buf, int nbuf)
9085 {
9086   printf(2, "$ ");
9087   memset(buf, 0, nbuf);
9088   gets(buf, nbuf);
9089   if(buf[0] == 0) // EOF
9090     return -1;
9091   return 0;
9092 }
9093 
9094 
9095 
9096 
9097 
9098 
9099 
9100 int
9101 main(void)
9102 {
9103   static char buf[100];
9104   int fd;
9105 
9106   // Ensure that three file descriptors are open.
9107   while((fd = open("console", O_RDWR)) >= 0){
9108     if(fd >= 3){
9109       close(fd);
9110       break;
9111     }
9112   }
9113 
9114   // Read and run input commands.
9115   while(getcmd(buf, sizeof(buf)) >= 0){
9116     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
9117       // Chdir must be called by the parent, not the child.
9118       buf[strlen(buf)-1] = 0;  // chop \n
9119       if(chdir(buf+3) < 0)
9120         printf(2, "cannot cd %s\n", buf+3);
9121       continue;
9122     }
9123     if(fork1() == 0)
9124       runcmd(parsecmd(buf));
9125     wait();
9126   }
9127   exit();
9128 }
9129 
9130 void
9131 panic(char *s)
9132 {
9133   printf(2, "%s\n", s);
9134   exit();
9135 }
9136 
9137 int
9138 fork1(void)
9139 {
9140   int pid;
9141 
9142   pid = fork();
9143   if(pid == -1)
9144     panic("fork");
9145   return pid;
9146 }
9147 
9148 
9149 
9150 // Constructors
9151 
9152 struct cmd*
9153 execcmd(void)
9154 {
9155   struct execcmd *cmd;
9156 
9157   cmd = malloc(sizeof(*cmd));
9158   memset(cmd, 0, sizeof(*cmd));
9159   cmd->type = EXEC;
9160   return (struct cmd*)cmd;
9161 }
9162 
9163 struct cmd*
9164 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
9165 {
9166   struct redircmd *cmd;
9167 
9168   cmd = malloc(sizeof(*cmd));
9169   memset(cmd, 0, sizeof(*cmd));
9170   cmd->type = REDIR;
9171   cmd->cmd = subcmd;
9172   cmd->file = file;
9173   cmd->efile = efile;
9174   cmd->mode = mode;
9175   cmd->fd = fd;
9176   return (struct cmd*)cmd;
9177 }
9178 
9179 struct cmd*
9180 pipecmd(struct cmd *left, struct cmd *right)
9181 {
9182   struct pipecmd *cmd;
9183 
9184   cmd = malloc(sizeof(*cmd));
9185   memset(cmd, 0, sizeof(*cmd));
9186   cmd->type = PIPE;
9187   cmd->left = left;
9188   cmd->right = right;
9189   return (struct cmd*)cmd;
9190 }
9191 
9192 
9193 
9194 
9195 
9196 
9197 
9198 
9199 
9200 struct cmd*
9201 listcmd(struct cmd *left, struct cmd *right)
9202 {
9203   struct listcmd *cmd;
9204 
9205   cmd = malloc(sizeof(*cmd));
9206   memset(cmd, 0, sizeof(*cmd));
9207   cmd->type = LIST;
9208   cmd->left = left;
9209   cmd->right = right;
9210   return (struct cmd*)cmd;
9211 }
9212 
9213 struct cmd*
9214 backcmd(struct cmd *subcmd)
9215 {
9216   struct backcmd *cmd;
9217 
9218   cmd = malloc(sizeof(*cmd));
9219   memset(cmd, 0, sizeof(*cmd));
9220   cmd->type = BACK;
9221   cmd->cmd = subcmd;
9222   return (struct cmd*)cmd;
9223 }
9224 
9225 
9226 
9227 
9228 
9229 
9230 
9231 
9232 
9233 
9234 
9235 
9236 
9237 
9238 
9239 
9240 
9241 
9242 
9243 
9244 
9245 
9246 
9247 
9248 
9249 
9250 // Parsing
9251 
9252 char whitespace[] = " \t\r\n\v";
9253 char symbols[] = "<|>&;()";
9254 
9255 int
9256 gettoken(char **ps, char *es, char **q, char **eq)
9257 {
9258   char *s;
9259   int ret;
9260 
9261   s = *ps;
9262   while(s < es && strchr(whitespace, *s))
9263     s++;
9264   if(q)
9265     *q = s;
9266   ret = *s;
9267   switch(*s){
9268   case 0:
9269     break;
9270   case '|':
9271   case '(':
9272   case ')':
9273   case ';':
9274   case '&':
9275   case '<':
9276     s++;
9277     break;
9278   case '>':
9279     s++;
9280     if(*s == '>'){
9281       ret = '+';
9282       s++;
9283     }
9284     break;
9285   default:
9286     ret = 'a';
9287     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
9288       s++;
9289     break;
9290   }
9291   if(eq)
9292     *eq = s;
9293 
9294   while(s < es && strchr(whitespace, *s))
9295     s++;
9296   *ps = s;
9297   return ret;
9298 }
9299 
9300 int
9301 peek(char **ps, char *es, char *toks)
9302 {
9303   char *s;
9304 
9305   s = *ps;
9306   while(s < es && strchr(whitespace, *s))
9307     s++;
9308   *ps = s;
9309   return *s && strchr(toks, *s);
9310 }
9311 
9312 struct cmd *parseline(char**, char*);
9313 struct cmd *parsepipe(char**, char*);
9314 struct cmd *parseexec(char**, char*);
9315 struct cmd *nulterminate(struct cmd*);
9316 
9317 struct cmd*
9318 parsecmd(char *s)
9319 {
9320   char *es;
9321   struct cmd *cmd;
9322 
9323   es = s + strlen(s);
9324   cmd = parseline(&s, es);
9325   peek(&s, es, "");
9326   if(s != es){
9327     printf(2, "leftovers: %s\n", s);
9328     panic("syntax");
9329   }
9330   nulterminate(cmd);
9331   return cmd;
9332 }
9333 
9334 struct cmd*
9335 parseline(char **ps, char *es)
9336 {
9337   struct cmd *cmd;
9338 
9339   cmd = parsepipe(ps, es);
9340   while(peek(ps, es, "&")){
9341     gettoken(ps, es, 0, 0);
9342     cmd = backcmd(cmd);
9343   }
9344   if(peek(ps, es, ";")){
9345     gettoken(ps, es, 0, 0);
9346     cmd = listcmd(cmd, parseline(ps, es));
9347   }
9348   return cmd;
9349 }
9350 struct cmd*
9351 parsepipe(char **ps, char *es)
9352 {
9353   struct cmd *cmd;
9354 
9355   cmd = parseexec(ps, es);
9356   if(peek(ps, es, "|")){
9357     gettoken(ps, es, 0, 0);
9358     cmd = pipecmd(cmd, parsepipe(ps, es));
9359   }
9360   return cmd;
9361 }
9362 
9363 struct cmd*
9364 parseredirs(struct cmd *cmd, char **ps, char *es)
9365 {
9366   int tok;
9367   char *q, *eq;
9368 
9369   while(peek(ps, es, "<>")){
9370     tok = gettoken(ps, es, 0, 0);
9371     if(gettoken(ps, es, &q, &eq) != 'a')
9372       panic("missing file for redirection");
9373     switch(tok){
9374     case '<':
9375       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
9376       break;
9377     case '>':
9378       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
9379       break;
9380     case '+':  // >>
9381       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
9382       break;
9383     }
9384   }
9385   return cmd;
9386 }
9387 
9388 
9389 
9390 
9391 
9392 
9393 
9394 
9395 
9396 
9397 
9398 
9399 
9400 struct cmd*
9401 parseblock(char **ps, char *es)
9402 {
9403   struct cmd *cmd;
9404 
9405   if(!peek(ps, es, "("))
9406     panic("parseblock");
9407   gettoken(ps, es, 0, 0);
9408   cmd = parseline(ps, es);
9409   if(!peek(ps, es, ")"))
9410     panic("syntax - missing )");
9411   gettoken(ps, es, 0, 0);
9412   cmd = parseredirs(cmd, ps, es);
9413   return cmd;
9414 }
9415 
9416 struct cmd*
9417 parseexec(char **ps, char *es)
9418 {
9419   char *q, *eq;
9420   int tok, argc;
9421   struct execcmd *cmd;
9422   struct cmd *ret;
9423 
9424   if(peek(ps, es, "("))
9425     return parseblock(ps, es);
9426 
9427   ret = execcmd();
9428   cmd = (struct execcmd*)ret;
9429 
9430   argc = 0;
9431   ret = parseredirs(ret, ps, es);
9432   while(!peek(ps, es, "|)&;")){
9433     if((tok=gettoken(ps, es, &q, &eq)) == 0)
9434       break;
9435     if(tok != 'a')
9436       panic("syntax");
9437     cmd->argv[argc] = q;
9438     cmd->eargv[argc] = eq;
9439     argc++;
9440     if(argc >= MAXARGS)
9441       panic("too many args");
9442     ret = parseredirs(ret, ps, es);
9443   }
9444   cmd->argv[argc] = 0;
9445   cmd->eargv[argc] = 0;
9446   return ret;
9447 }
9448 
9449 
9450 // NUL-terminate all the counted strings.
9451 struct cmd*
9452 nulterminate(struct cmd *cmd)
9453 {
9454   int i;
9455   struct backcmd *bcmd;
9456   struct execcmd *ecmd;
9457   struct listcmd *lcmd;
9458   struct pipecmd *pcmd;
9459   struct redircmd *rcmd;
9460 
9461   if(cmd == 0)
9462     return 0;
9463 
9464   switch(cmd->type){
9465   case EXEC:
9466     ecmd = (struct execcmd*)cmd;
9467     for(i=0; ecmd->argv[i]; i++)
9468       *ecmd->eargv[i] = 0;
9469     break;
9470 
9471   case REDIR:
9472     rcmd = (struct redircmd*)cmd;
9473     nulterminate(rcmd->cmd);
9474     *rcmd->efile = 0;
9475     break;
9476 
9477   case PIPE:
9478     pcmd = (struct pipecmd*)cmd;
9479     nulterminate(pcmd->left);
9480     nulterminate(pcmd->right);
9481     break;
9482 
9483   case LIST:
9484     lcmd = (struct listcmd*)cmd;
9485     nulterminate(lcmd->left);
9486     nulterminate(lcmd->right);
9487     break;
9488 
9489   case BACK:
9490     bcmd = (struct backcmd*)cmd;
9491     nulterminate(bcmd->cmd);
9492     break;
9493   }
9494   return cmd;
9495 }
9496 
9497 
9498 
9499 
