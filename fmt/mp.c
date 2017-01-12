7250 // Multiprocessor support
7251 // Search memory for MP description structures.
7252 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
7253 
7254 #include "types.h"
7255 #include "defs.h"
7256 #include "param.h"
7257 #include "memlayout.h"
7258 #include "mp.h"
7259 #include "x86.h"
7260 #include "mmu.h"
7261 #include "proc.h"
7262 
7263 struct cpu cpus[NCPU];
7264 int ismp;
7265 int ncpu;
7266 uchar ioapicid;
7267 
7268 static uchar
7269 sum(uchar *addr, int len)
7270 {
7271   int i, sum;
7272 
7273   sum = 0;
7274   for(i=0; i<len; i++)
7275     sum += addr[i];
7276   return sum;
7277 }
7278 
7279 // Look for an MP structure in the len bytes at addr.
7280 static struct mp*
7281 mpsearch1(uint a, int len)
7282 {
7283   uchar *e, *p, *addr;
7284 
7285   addr = P2V(a);
7286   e = addr+len;
7287   for(p = addr; p < e; p += sizeof(struct mp))
7288     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
7289       return (struct mp*)p;
7290   return 0;
7291 }
7292 
7293 
7294 
7295 
7296 
7297 
7298 
7299 
7300 // Search for the MP Floating Pointer Structure, which according to the
7301 // spec is in one of the following three locations:
7302 // 1) in the first KB of the EBDA;
7303 // 2) in the last KB of system base memory;
7304 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
7305 static struct mp*
7306 mpsearch(void)
7307 {
7308   uchar *bda;
7309   uint p;
7310   struct mp *mp;
7311 
7312   bda = (uchar *) P2V(0x400);
7313   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
7314     if((mp = mpsearch1(p, 1024)))
7315       return mp;
7316   } else {
7317     p = ((bda[0x14]<<8)|bda[0x13])*1024;
7318     if((mp = mpsearch1(p-1024, 1024)))
7319       return mp;
7320   }
7321   return mpsearch1(0xF0000, 0x10000);
7322 }
7323 
7324 // Search for an MP configuration table.  For now,
7325 // don't accept the default configurations (physaddr == 0).
7326 // Check for correct signature, calculate the checksum and,
7327 // if correct, check the version.
7328 // To do: check extended table checksum.
7329 static struct mpconf*
7330 mpconfig(struct mp **pmp)
7331 {
7332   struct mpconf *conf;
7333   struct mp *mp;
7334 
7335   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
7336     return 0;
7337   conf = (struct mpconf*) P2V((uint) mp->physaddr);
7338   if(memcmp(conf, "PCMP", 4) != 0)
7339     return 0;
7340   if(conf->version != 1 && conf->version != 4)
7341     return 0;
7342   if(sum((uchar*)conf, conf->length) != 0)
7343     return 0;
7344   *pmp = mp;
7345   return conf;
7346 }
7347 
7348 
7349 
7350 void
7351 mpinit(void)
7352 {
7353   uchar *p, *e;
7354   struct mp *mp;
7355   struct mpconf *conf;
7356   struct mpproc *proc;
7357   struct mpioapic *ioapic;
7358 
7359   if((conf = mpconfig(&mp)) == 0)
7360     return;
7361   ismp = 1;
7362   lapic = (uint*)conf->lapicaddr;
7363   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
7364     switch(*p){
7365     case MPPROC:
7366       proc = (struct mpproc*)p;
7367       if(ncpu < NCPU) {
7368         cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
7369         ncpu++;
7370       }
7371       p += sizeof(struct mpproc);
7372       continue;
7373     case MPIOAPIC:
7374       ioapic = (struct mpioapic*)p;
7375       ioapicid = ioapic->apicno;
7376       p += sizeof(struct mpioapic);
7377       continue;
7378     case MPBUS:
7379     case MPIOINTR:
7380     case MPLINTR:
7381       p += 8;
7382       continue;
7383     default:
7384       ismp = 0;
7385       break;
7386     }
7387   }
7388   if(!ismp){
7389     // Didn't like what we found; fall back to no MP.
7390     ncpu = 1;
7391     lapic = 0;
7392     ioapicid = 0;
7393     return;
7394   }
7395 
7396 
7397 
7398 
7399 
7400   if(mp->imcrp){
7401     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
7402     // But it would on real hardware.
7403     outb(0x22, 0x70);   // Select IMCR
7404     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
7405   }
7406 }
7407 
7408 
7409 
7410 
7411 
7412 
7413 
7414 
7415 
7416 
7417 
7418 
7419 
7420 
7421 
7422 
7423 
7424 
7425 
7426 
7427 
7428 
7429 
7430 
7431 
7432 
7433 
7434 
7435 
7436 
7437 
7438 
7439 
7440 
7441 
7442 
7443 
7444 
7445 
7446 
7447 
7448 
7449 
