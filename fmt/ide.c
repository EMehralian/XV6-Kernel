4250 // Simple PIO-based (non-DMA) IDE driver code.
4251 
4252 #include "types.h"
4253 #include "defs.h"
4254 #include "param.h"
4255 #include "memlayout.h"
4256 #include "mmu.h"
4257 #include "proc.h"
4258 #include "x86.h"
4259 #include "traps.h"
4260 #include "spinlock.h"
4261 #include "sleeplock.h"
4262 #include "fs.h"
4263 #include "buf.h"
4264 
4265 #define SECTOR_SIZE   512
4266 #define IDE_BSY       0x80
4267 #define IDE_DRDY      0x40
4268 #define IDE_DF        0x20
4269 #define IDE_ERR       0x01
4270 
4271 #define IDE_CMD_READ  0x20
4272 #define IDE_CMD_WRITE 0x30
4273 #define IDE_CMD_RDMUL 0xc4
4274 #define IDE_CMD_WRMUL 0xc5
4275 
4276 // idequeue points to the buf now being read/written to the disk.
4277 // idequeue->qnext points to the next buf to be processed.
4278 // You must hold idelock while manipulating queue.
4279 
4280 static struct spinlock idelock;
4281 static struct buf *idequeue;
4282 
4283 static int havedisk1;
4284 static void idestart(struct buf*);
4285 
4286 // Wait for IDE disk to become ready.
4287 static int
4288 idewait(int checkerr)
4289 {
4290   int r;
4291 
4292   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
4293     ;
4294   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
4295     return -1;
4296   return 0;
4297 }
4298 
4299 
4300 void
4301 ideinit(void)
4302 {
4303   int i;
4304 
4305   initlock(&idelock, "ide");
4306   picenable(IRQ_IDE);
4307   ioapicenable(IRQ_IDE, ncpu - 1);
4308   idewait(0);
4309 
4310   // Check if disk 1 is present
4311   outb(0x1f6, 0xe0 | (1<<4));
4312   for(i=0; i<1000; i++){
4313     if(inb(0x1f7) != 0){
4314       havedisk1 = 1;
4315       break;
4316     }
4317   }
4318 
4319   // Switch back to disk 0.
4320   outb(0x1f6, 0xe0 | (0<<4));
4321 }
4322 
4323 // Start the request for b.  Caller must hold idelock.
4324 static void
4325 idestart(struct buf *b)
4326 {
4327   if(b == 0)
4328     panic("idestart");
4329   if(b->blockno >= FSSIZE)
4330     panic("incorrect blockno");
4331   int sector_per_block =  BSIZE/SECTOR_SIZE;
4332   int sector = b->blockno * sector_per_block;
4333   int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
4334   int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
4335 
4336   if (sector_per_block > 7) panic("idestart");
4337 
4338   idewait(0);
4339   outb(0x3f6, 0);  // generate interrupt
4340   outb(0x1f2, sector_per_block);  // number of sectors
4341   outb(0x1f3, sector & 0xff);
4342   outb(0x1f4, (sector >> 8) & 0xff);
4343   outb(0x1f5, (sector >> 16) & 0xff);
4344   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
4345   if(b->flags & B_DIRTY){
4346     outb(0x1f7, write_cmd);
4347     outsl(0x1f0, b->data, BSIZE/4);
4348   } else {
4349     outb(0x1f7, read_cmd);
4350   }
4351 }
4352 
4353 // Interrupt handler.
4354 void
4355 ideintr(void)
4356 {
4357   struct buf *b;
4358 
4359   // First queued buffer is the active request.
4360   acquire(&idelock);
4361   if((b = idequeue) == 0){
4362     release(&idelock);
4363     // cprintf("spurious IDE interrupt\n");
4364     return;
4365   }
4366   idequeue = b->qnext;
4367 
4368   // Read data if needed.
4369   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4370     insl(0x1f0, b->data, BSIZE/4);
4371 
4372   // Wake process waiting for this buf.
4373   b->flags |= B_VALID;
4374   b->flags &= ~B_DIRTY;
4375   wakeup(b);
4376 
4377   // Start disk on next buf in queue.
4378   if(idequeue != 0)
4379     idestart(idequeue);
4380 
4381   release(&idelock);
4382 }
4383 
4384 
4385 
4386 
4387 
4388 
4389 
4390 
4391 
4392 
4393 
4394 
4395 
4396 
4397 
4398 
4399 
4400 // Sync buf with disk.
4401 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4402 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4403 void
4404 iderw(struct buf *b)
4405 {
4406   struct buf **pp;
4407 
4408   if(!holdingsleep(&b->lock))
4409     panic("iderw: buf not locked");
4410   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4411     panic("iderw: nothing to do");
4412   if(b->dev != 0 && !havedisk1)
4413     panic("iderw: ide disk 1 not present");
4414 
4415   acquire(&idelock);  //DOC:acquire-lock
4416 
4417   // Append b to idequeue.
4418   b->qnext = 0;
4419   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4420     ;
4421   *pp = b;
4422 
4423   // Start disk if necessary.
4424   if(idequeue == b)
4425     idestart(b);
4426 
4427   // Wait for request to finish.
4428   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4429     sleep(b, &idelock);
4430   }
4431 
4432   release(&idelock);
4433 }
4434 
4435 
4436 
4437 
4438 
4439 
4440 
4441 
4442 
4443 
4444 
4445 
4446 
4447 
4448 
4449 
