8200 #include "types.h"
8201 #include "x86.h"
8202 #include "defs.h"
8203 #include "kbd.h"
8204 
8205 int
8206 kbdgetc(void)
8207 {
8208   static uint shift;
8209   static uchar *charcode[4] = {
8210     normalmap, shiftmap, ctlmap, ctlmap
8211   };
8212   uint st, data, c;
8213 
8214   st = inb(KBSTATP);
8215   if((st & KBS_DIB) == 0)
8216     return -1;
8217   data = inb(KBDATAP);
8218 
8219   if(data == 0xE0){
8220     shift |= E0ESC;
8221     return 0;
8222   } else if(data & 0x80){
8223     // Key released
8224     data = (shift & E0ESC ? data : data & 0x7F);
8225     shift &= ~(shiftcode[data] | E0ESC);
8226     return 0;
8227   } else if(shift & E0ESC){
8228     // Last character was an E0 escape; or with 0x80
8229     data |= 0x80;
8230     shift &= ~E0ESC;
8231   }
8232 
8233   shift |= shiftcode[data];
8234   shift ^= togglecode[data];
8235   c = charcode[shift & (CTL | SHIFT)][data];
8236   if(shift & CAPSLOCK){
8237     if('a' <= c && c <= 'z')
8238       c += 'A' - 'a';
8239     else if('A' <= c && c <= 'Z')
8240       c += 'a' - 'A';
8241   }
8242   return c;
8243 }
8244 
8245 void
8246 kbdintr(void)
8247 {
8248   consoleintr(kbdgetc);
8249 }
