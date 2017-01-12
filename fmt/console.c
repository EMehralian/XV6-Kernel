8250 // Console input and output.
8251 // Input is from the keyboard or serial port.
8252 // Output is written to the screen and serial port.
8253 
8254 #include "types.h"
8255 #include "defs.h"
8256 #include "param.h"
8257 #include "traps.h"
8258 #include "spinlock.h"
8259 #include "sleeplock.h"
8260 #include "fs.h"
8261 #include "file.h"
8262 #include "memlayout.h"
8263 #include "mmu.h"
8264 #include "proc.h"
8265 #include "x86.h"
8266 
8267 static void consputc(int);
8268 
8269 static int panicked = 0;
8270 
8271 static struct {
8272   struct spinlock lock;
8273   int locking;
8274 } cons;
8275 
8276 static void
8277 printint(int xx, int base, int sign)
8278 {
8279   static char digits[] = "0123456789abcdef";
8280   char buf[16];
8281   int i;
8282   uint x;
8283 
8284   if(sign && (sign = xx < 0))
8285     x = -xx;
8286   else
8287     x = xx;
8288 
8289   i = 0;
8290   do{
8291     buf[i++] = digits[x % base];
8292   }while((x /= base) != 0);
8293 
8294   if(sign)
8295     buf[i++] = '-';
8296 
8297   while(--i >= 0)
8298     consputc(buf[i]);
8299 }
8300 
8301 
8302 
8303 
8304 
8305 
8306 
8307 
8308 
8309 
8310 
8311 
8312 
8313 
8314 
8315 
8316 
8317 
8318 
8319 
8320 
8321 
8322 
8323 
8324 
8325 
8326 
8327 
8328 
8329 
8330 
8331 
8332 
8333 
8334 
8335 
8336 
8337 
8338 
8339 
8340 
8341 
8342 
8343 
8344 
8345 
8346 
8347 
8348 
8349 
8350 // Print to the console. only understands %d, %x, %p, %s.
8351 void
8352 cprintf(char *fmt, ...)
8353 {
8354   int i, c, locking;
8355   uint *argp;
8356   char *s;
8357 
8358   locking = cons.locking;
8359   if(locking)
8360     acquire(&cons.lock);
8361 
8362   if (fmt == 0)
8363     panic("null fmt");
8364 
8365   argp = (uint*)(void*)(&fmt + 1);
8366   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
8367     if(c != '%'){
8368       consputc(c);
8369       continue;
8370     }
8371     c = fmt[++i] & 0xff;
8372     if(c == 0)
8373       break;
8374     switch(c){
8375     case 'd':
8376       printint(*argp++, 10, 1);
8377       break;
8378     case 'x':
8379     case 'p':
8380       printint(*argp++, 16, 0);
8381       break;
8382     case 's':
8383       if((s = (char*)*argp++) == 0)
8384         s = "(null)";
8385       for(; *s; s++)
8386         consputc(*s);
8387       break;
8388     case '%':
8389       consputc('%');
8390       break;
8391     default:
8392       // Print unknown % sequence to draw attention.
8393       consputc('%');
8394       consputc(c);
8395       break;
8396     }
8397   }
8398 
8399 
8400   if(locking)
8401     release(&cons.lock);
8402 }
8403 
8404 void
8405 panic(char *s)
8406 {
8407   int i;
8408   uint pcs[10];
8409 
8410   cli();
8411   cons.locking = 0;
8412   cprintf("cpu with apicid %d: panic: ", cpu->apicid);
8413   cprintf(s);
8414   cprintf("\n");
8415   getcallerpcs(&s, pcs);
8416   for(i=0; i<10; i++)
8417     cprintf(" %p", pcs[i]);
8418   panicked = 1; // freeze other CPU
8419   for(;;)
8420     ;
8421 }
8422 
8423 
8424 
8425 
8426 
8427 
8428 
8429 
8430 
8431 
8432 
8433 
8434 
8435 
8436 
8437 
8438 
8439 
8440 
8441 
8442 
8443 
8444 
8445 
8446 
8447 
8448 
8449 
8450 #define BACKSPACE 0x100
8451 #define CRTPORT 0x3d4
8452 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
8453 
8454 static void
8455 cgaputc(int c)
8456 {
8457   int pos;
8458 
8459   // Cursor position: col + 80*row.
8460   outb(CRTPORT, 14);
8461   pos = inb(CRTPORT+1) << 8;
8462   outb(CRTPORT, 15);
8463   pos |= inb(CRTPORT+1);
8464 
8465   if(c == '\n')
8466     pos += 80 - pos%80;
8467   else if(c == BACKSPACE){
8468     if(pos > 0) --pos;
8469   } else
8470     crt[pos++] = (c&0xff) | 0x0700;  // black on white
8471 
8472   if(pos < 0 || pos > 25*80)
8473     panic("pos under/overflow");
8474 
8475   if((pos/80) >= 24){  // Scroll up.
8476     memmove(crt, crt+80, sizeof(crt[0])*23*80);
8477     pos -= 80;
8478     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
8479   }
8480 
8481   outb(CRTPORT, 14);
8482   outb(CRTPORT+1, pos>>8);
8483   outb(CRTPORT, 15);
8484   outb(CRTPORT+1, pos);
8485   crt[pos] = ' ' | 0x0700;
8486 }
8487 
8488 
8489 
8490 
8491 
8492 
8493 
8494 
8495 
8496 
8497 
8498 
8499 
8500 void
8501 consputc(int c)
8502 {
8503   if(panicked){
8504     cli();
8505     for(;;)
8506       ;
8507   }
8508 
8509   if(c == BACKSPACE){
8510     uartputc('\b'); uartputc(' '); uartputc('\b');
8511   } else
8512     uartputc(c);
8513   cgaputc(c);
8514 }
8515 
8516 #define INPUT_BUF 128
8517 struct {
8518   char buf[INPUT_BUF];
8519   uint r;  // Read index
8520   uint w;  // Write index
8521   uint e;  // Edit index
8522 } input;
8523 
8524 #define C(x)  ((x)-'@')  // Control-x
8525 
8526 void
8527 consoleintr(int (*getc)(void))
8528 {
8529   int c, doprocdump = 0;
8530 
8531   acquire(&cons.lock);
8532   while((c = getc()) >= 0){
8533     switch(c){
8534     case C('P'):  // Process listing.
8535       // procdump() locks cons.lock indirectly; invoke later
8536       doprocdump = 1;
8537       break;
8538     case C('U'):  // Kill line.
8539       while(input.e != input.w &&
8540             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
8541         input.e--;
8542         consputc(BACKSPACE);
8543       }
8544       break;
8545     case C('H'): case '\x7f':  // Backspace
8546       if(input.e != input.w){
8547         input.e--;
8548         consputc(BACKSPACE);
8549       }
8550       break;
8551     default:
8552       if(c != 0 && input.e-input.r < INPUT_BUF){
8553         c = (c == '\r') ? '\n' : c;
8554         input.buf[input.e++ % INPUT_BUF] = c;
8555         consputc(c);
8556         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
8557           input.w = input.e;
8558           wakeup(&input.r);
8559         }
8560       }
8561       break;
8562     }
8563   }
8564   release(&cons.lock);
8565   if(doprocdump) {
8566     procdump();  // now call procdump() wo. cons.lock held
8567   }
8568 }
8569 
8570 int
8571 consoleread(struct inode *ip, char *dst, int n)
8572 {
8573   uint target;
8574   int c;
8575 
8576   iunlock(ip);
8577   target = n;
8578   acquire(&cons.lock);
8579   while(n > 0){
8580     while(input.r == input.w){
8581       if(proc->killed){
8582         release(&cons.lock);
8583         ilock(ip);
8584         return -1;
8585       }
8586       sleep(&input.r, &cons.lock);
8587     }
8588     c = input.buf[input.r++ % INPUT_BUF];
8589     if(c == C('D')){  // EOF
8590       if(n < target){
8591         // Save ^D for next time, to make sure
8592         // caller gets a 0-byte result.
8593         input.r--;
8594       }
8595       break;
8596     }
8597     *dst++ = c;
8598     --n;
8599     if(c == '\n')
8600       break;
8601   }
8602   release(&cons.lock);
8603   ilock(ip);
8604 
8605   return target - n;
8606 }
8607 
8608 int
8609 consolewrite(struct inode *ip, char *buf, int n)
8610 {
8611   int i;
8612 
8613   iunlock(ip);
8614   acquire(&cons.lock);
8615   for(i = 0; i < n; i++)
8616     consputc(buf[i] & 0xff);
8617   release(&cons.lock);
8618   ilock(ip);
8619 
8620   return n;
8621 }
8622 
8623 void
8624 consoleinit(void)
8625 {
8626   initlock(&cons.lock, "console");
8627 
8628   devsw[CONSOLE].write = consolewrite;
8629   devsw[CONSOLE].read = consoleread;
8630   cons.locking = 1;
8631 
8632   picenable(IRQ_KBD);
8633   ioapicenable(IRQ_KBD, 0);
8634 }
8635 
8636 
8637 
8638 
8639 
8640 
8641 
8642 
8643 
8644 
8645 
8646 
8647 
8648 
8649 
