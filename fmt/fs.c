5000 // File system implementation.  Five layers:
5001 //   + Blocks: allocator for raw disk blocks.
5002 //   + Log: crash recovery for multi-step updates.
5003 //   + Files: inode allocator, reading, writing, metadata.
5004 //   + Directories: inode with special contents (list of other inodes!)
5005 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
5006 //
5007 // This file contains the low-level file system manipulation
5008 // routines.  The (higher-level) system call implementations
5009 // are in sysfile.c.
5010 
5011 #include "types.h"
5012 #include "defs.h"
5013 #include "param.h"
5014 #include "stat.h"
5015 #include "mmu.h"
5016 #include "proc.h"
5017 #include "spinlock.h"
5018 #include "sleeplock.h"
5019 #include "fs.h"
5020 #include "buf.h"
5021 #include "file.h"
5022 
5023 #define min(a, b) ((a) < (b) ? (a) : (b))
5024 static void itrunc(struct inode*);
5025 // there should be one superblock per disk device, but we run with
5026 // only one device
5027 struct superblock sb;
5028 
5029 // Read the super block.
5030 void
5031 readsb(int dev, struct superblock *sb)
5032 {
5033   struct buf *bp;
5034 
5035   bp = bread(dev, 1);
5036   memmove(sb, bp->data, sizeof(*sb));
5037   brelse(bp);
5038 }
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Zero a block.
5051 static void
5052 bzero(int dev, int bno)
5053 {
5054   struct buf *bp;
5055 
5056   bp = bread(dev, bno);
5057   memset(bp->data, 0, BSIZE);
5058   log_write(bp);
5059   brelse(bp);
5060 }
5061 
5062 // Blocks.
5063 
5064 // Allocate a zeroed disk block.
5065 static uint
5066 balloc(uint dev)
5067 {
5068   int b, bi, m;
5069   struct buf *bp;
5070 
5071   bp = 0;
5072   for(b = 0; b < sb.size; b += BPB){
5073     bp = bread(dev, BBLOCK(b, sb));
5074     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
5075       m = 1 << (bi % 8);
5076       if((bp->data[bi/8] & m) == 0){  // Is block free?
5077         bp->data[bi/8] |= m;  // Mark block in use.
5078         log_write(bp);
5079         brelse(bp);
5080         bzero(dev, b + bi);
5081         return b + bi;
5082       }
5083     }
5084     brelse(bp);
5085   }
5086   panic("balloc: out of blocks");
5087 }
5088 
5089 
5090 
5091 
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
5100 // Free a disk block.
5101 static void
5102 bfree(int dev, uint b)
5103 {
5104   struct buf *bp;
5105   int bi, m;
5106 
5107   readsb(dev, &sb);
5108   bp = bread(dev, BBLOCK(b, sb));
5109   bi = b % BPB;
5110   m = 1 << (bi % 8);
5111   if((bp->data[bi/8] & m) == 0)
5112     panic("freeing free block");
5113   bp->data[bi/8] &= ~m;
5114   log_write(bp);
5115   brelse(bp);
5116 }
5117 
5118 // Inodes.
5119 //
5120 // An inode describes a single unnamed file.
5121 // The inode disk structure holds metadata: the file's type,
5122 // its size, the number of links referring to it, and the
5123 // list of blocks holding the file's content.
5124 //
5125 // The inodes are laid out sequentially on disk at
5126 // sb.startinode. Each inode has a number, indicating its
5127 // position on the disk.
5128 //
5129 // The kernel keeps a cache of in-use inodes in memory
5130 // to provide a place for synchronizing access
5131 // to inodes used by multiple processes. The cached
5132 // inodes include book-keeping information that is
5133 // not stored on disk: ip->ref and ip->flags.
5134 //
5135 // An inode and its in-memory represtative go through a
5136 // sequence of states before they can be used by the
5137 // rest of the file system code.
5138 //
5139 // * Allocation: an inode is allocated if its type (on disk)
5140 //   is non-zero. ialloc() allocates, iput() frees if
5141 //   the link count has fallen to zero.
5142 //
5143 // * Referencing in cache: an entry in the inode cache
5144 //   is free if ip->ref is zero. Otherwise ip->ref tracks
5145 //   the number of in-memory pointers to the entry (open
5146 //   files and current directories). iget() to find or
5147 //   create a cache entry and increment its ref, iput()
5148 //   to decrement ref.
5149 //
5150 // * Valid: the information (type, size, &c) in an inode
5151 //   cache entry is only correct when the I_VALID bit
5152 //   is set in ip->flags. ilock() reads the inode from
5153 //   the disk and sets I_VALID, while iput() clears
5154 //   I_VALID if ip->ref has fallen to zero.
5155 //
5156 // * Locked: file system code may only examine and modify
5157 //   the information in an inode and its content if it
5158 //   has first locked the inode.
5159 //
5160 // Thus a typical sequence is:
5161 //   ip = iget(dev, inum)
5162 //   ilock(ip)
5163 //   ... examine and modify ip->xxx ...
5164 //   iunlock(ip)
5165 //   iput(ip)
5166 //
5167 // ilock() is separate from iget() so that system calls can
5168 // get a long-term reference to an inode (as for an open file)
5169 // and only lock it for short periods (e.g., in read()).
5170 // The separation also helps avoid deadlock and races during
5171 // pathname lookup. iget() increments ip->ref so that the inode
5172 // stays cached and pointers to it remain valid.
5173 //
5174 // Many internal file system functions expect the caller to
5175 // have locked the inodes involved; this lets callers create
5176 // multi-step atomic operations.
5177 
5178 struct {
5179   struct spinlock lock;
5180   struct inode inode[NINODE];
5181 } icache;
5182 
5183 void
5184 iinit(int dev)
5185 {
5186   int i = 0;
5187 
5188   initlock(&icache.lock, "icache");
5189   for(i = 0; i < NINODE; i++) {
5190     initsleeplock(&icache.inode[i].lock, "inode");
5191   }
5192 
5193   readsb(dev, &sb);
5194   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
5195  inodestart %d bmap start %d\n", sb.size, sb.nblocks,
5196           sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
5197           sb.bmapstart);
5198 }
5199 
5200 static struct inode* iget(uint dev, uint inum);
5201 
5202 
5203 
5204 
5205 
5206 
5207 
5208 
5209 
5210 
5211 
5212 
5213 
5214 
5215 
5216 
5217 
5218 
5219 
5220 
5221 
5222 
5223 
5224 
5225 
5226 
5227 
5228 
5229 
5230 
5231 
5232 
5233 
5234 
5235 
5236 
5237 
5238 
5239 
5240 
5241 
5242 
5243 
5244 
5245 
5246 
5247 
5248 
5249 
5250 // Allocate a new inode with the given type on device dev.
5251 // A free inode has a type of zero.
5252 struct inode*
5253 ialloc(uint dev, short type)
5254 {
5255   int inum;
5256   struct buf *bp;
5257   struct dinode *dip;
5258 
5259   for(inum = 1; inum < sb.ninodes; inum++){
5260     bp = bread(dev, IBLOCK(inum, sb));
5261     dip = (struct dinode*)bp->data + inum%IPB;
5262     if(dip->type == 0){  // a free inode
5263       memset(dip, 0, sizeof(*dip));
5264       dip->type = type;
5265       log_write(bp);   // mark it allocated on the disk
5266       brelse(bp);
5267       return iget(dev, inum);
5268     }
5269     brelse(bp);
5270   }
5271   panic("ialloc: no inodes");
5272 }
5273 
5274 // Copy a modified in-memory inode to disk.
5275 void
5276 iupdate(struct inode *ip)
5277 {
5278   struct buf *bp;
5279   struct dinode *dip;
5280 
5281   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5282   dip = (struct dinode*)bp->data + ip->inum%IPB;
5283   dip->type = ip->type;
5284   dip->major = ip->major;
5285   dip->minor = ip->minor;
5286   dip->nlink = ip->nlink;
5287   dip->size = ip->size;
5288   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
5289   log_write(bp);
5290   brelse(bp);
5291 }
5292 
5293 
5294 
5295 
5296 
5297 
5298 
5299 
5300 // Find the inode with number inum on device dev
5301 // and return the in-memory copy. Does not lock
5302 // the inode and does not read it from disk.
5303 static struct inode*
5304 iget(uint dev, uint inum)
5305 {
5306   struct inode *ip, *empty;
5307 
5308   acquire(&icache.lock);
5309 
5310   // Is the inode already cached?
5311   empty = 0;
5312   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
5313     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
5314       ip->ref++;
5315       release(&icache.lock);
5316       return ip;
5317     }
5318     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
5319       empty = ip;
5320   }
5321 
5322   // Recycle an inode cache entry.
5323   if(empty == 0)
5324     panic("iget: no inodes");
5325 
5326   ip = empty;
5327   ip->dev = dev;
5328   ip->inum = inum;
5329   ip->ref = 1;
5330   ip->flags = 0;
5331   release(&icache.lock);
5332 
5333   return ip;
5334 }
5335 
5336 // Increment reference count for ip.
5337 // Returns ip to enable ip = idup(ip1) idiom.
5338 struct inode*
5339 idup(struct inode *ip)
5340 {
5341   acquire(&icache.lock);
5342   ip->ref++;
5343   release(&icache.lock);
5344   return ip;
5345 }
5346 
5347 
5348 
5349 
5350 // Lock the given inode.
5351 // Reads the inode from disk if necessary.
5352 void
5353 ilock(struct inode *ip)
5354 {
5355   struct buf *bp;
5356   struct dinode *dip;
5357 
5358   if(ip == 0 || ip->ref < 1)
5359     panic("ilock");
5360 
5361   acquiresleep(&ip->lock);
5362 
5363   if(!(ip->flags & I_VALID)){
5364     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5365     dip = (struct dinode*)bp->data + ip->inum%IPB;
5366     ip->type = dip->type;
5367     ip->major = dip->major;
5368     ip->minor = dip->minor;
5369     ip->nlink = dip->nlink;
5370     ip->size = dip->size;
5371     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
5372     brelse(bp);
5373     ip->flags |= I_VALID;
5374     if(ip->type == 0)
5375       panic("ilock: no type");
5376   }
5377 }
5378 
5379 // Unlock the given inode.
5380 void
5381 iunlock(struct inode *ip)
5382 {
5383   if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
5384     panic("iunlock");
5385 
5386   releasesleep(&ip->lock);
5387 }
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Drop a reference to an in-memory inode.
5401 // If that was the last reference, the inode cache entry can
5402 // be recycled.
5403 // If that was the last reference and the inode has no links
5404 // to it, free the inode (and its content) on disk.
5405 // All calls to iput() must be inside a transaction in
5406 // case it has to free the inode.
5407 void
5408 iput(struct inode *ip)
5409 {
5410   acquire(&icache.lock);
5411   if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
5412     // inode has no links and no other references: truncate and free.
5413     release(&icache.lock);
5414     itrunc(ip);
5415     ip->type = 0;
5416     iupdate(ip);
5417     acquire(&icache.lock);
5418     ip->flags = 0;
5419   }
5420   ip->ref--;
5421   release(&icache.lock);
5422 }
5423 
5424 // Common idiom: unlock, then put.
5425 void
5426 iunlockput(struct inode *ip)
5427 {
5428   iunlock(ip);
5429   iput(ip);
5430 }
5431 
5432 
5433 
5434 
5435 
5436 
5437 
5438 
5439 
5440 
5441 
5442 
5443 
5444 
5445 
5446 
5447 
5448 
5449 
5450 // Inode content
5451 //
5452 // The content (data) associated with each inode is stored
5453 // in blocks on the disk. The first NDIRECT block numbers
5454 // are listed in ip->addrs[].  The next NINDIRECT blocks are
5455 // listed in block ip->addrs[NDIRECT].
5456 
5457 // Return the disk block address of the nth block in inode ip.
5458 // If there is no such block, bmap allocates one.
5459 static uint
5460 bmap(struct inode *ip, uint bn)
5461 {
5462   uint addr, *a;
5463   struct buf *bp;
5464 
5465   if(bn < NDIRECT){
5466     if((addr = ip->addrs[bn]) == 0)
5467       ip->addrs[bn] = addr = balloc(ip->dev);
5468     return addr;
5469   }
5470   bn -= NDIRECT;
5471 
5472   if(bn < NINDIRECT){
5473     // Load indirect block, allocating if necessary.
5474     if((addr = ip->addrs[NDIRECT]) == 0)
5475       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
5476     bp = bread(ip->dev, addr);
5477     a = (uint*)bp->data;
5478     if((addr = a[bn]) == 0){
5479       a[bn] = addr = balloc(ip->dev);
5480       log_write(bp);
5481     }
5482     brelse(bp);
5483     return addr;
5484   }
5485 
5486   panic("bmap: out of range");
5487 }
5488 
5489 
5490 
5491 
5492 
5493 
5494 
5495 
5496 
5497 
5498 
5499 
5500 // Truncate inode (discard contents).
5501 // Only called when the inode has no links
5502 // to it (no directory entries referring to it)
5503 // and has no in-memory reference to it (is
5504 // not an open file or current directory).
5505 static void
5506 itrunc(struct inode *ip)
5507 {
5508   int i, j;
5509   struct buf *bp;
5510   uint *a;
5511 
5512   for(i = 0; i < NDIRECT; i++){
5513     if(ip->addrs[i]){
5514       bfree(ip->dev, ip->addrs[i]);
5515       ip->addrs[i] = 0;
5516     }
5517   }
5518 
5519   if(ip->addrs[NDIRECT]){
5520     bp = bread(ip->dev, ip->addrs[NDIRECT]);
5521     a = (uint*)bp->data;
5522     for(j = 0; j < NINDIRECT; j++){
5523       if(a[j])
5524         bfree(ip->dev, a[j]);
5525     }
5526     brelse(bp);
5527     bfree(ip->dev, ip->addrs[NDIRECT]);
5528     ip->addrs[NDIRECT] = 0;
5529   }
5530 
5531   ip->size = 0;
5532   iupdate(ip);
5533 }
5534 
5535 // Copy stat information from inode.
5536 void
5537 stati(struct inode *ip, struct stat *st)
5538 {
5539   st->dev = ip->dev;
5540   st->ino = ip->inum;
5541   st->type = ip->type;
5542   st->nlink = ip->nlink;
5543   st->size = ip->size;
5544 }
5545 
5546 
5547 
5548 
5549 
5550 // Read data from inode.
5551 int
5552 readi(struct inode *ip, char *dst, uint off, uint n)
5553 {
5554   uint tot, m;
5555   struct buf *bp;
5556 
5557   if(ip->type == T_DEV){
5558     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5559       return -1;
5560     return devsw[ip->major].read(ip, dst, n);
5561   }
5562 
5563   if(off > ip->size || off + n < off)
5564     return -1;
5565   if(off + n > ip->size)
5566     n = ip->size - off;
5567 
5568   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5569     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5570     m = min(n - tot, BSIZE - off%BSIZE);
5571     /*
5572     cprintf("data off %d:\n", off);
5573     for (int j = 0; j < min(m, 10); j++) {
5574       cprintf("%x ", bp->data[off%BSIZE+j]);
5575     }
5576     cprintf("\n");
5577     */
5578     memmove(dst, bp->data + off%BSIZE, m);
5579     brelse(bp);
5580   }
5581   return n;
5582 }
5583 
5584 
5585 
5586 
5587 
5588 
5589 
5590 
5591 
5592 
5593 
5594 
5595 
5596 
5597 
5598 
5599 
5600 // Write data to inode.
5601 int
5602 writei(struct inode *ip, char *src, uint off, uint n)
5603 {
5604   uint tot, m;
5605   struct buf *bp;
5606 
5607   if(ip->type == T_DEV){
5608     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5609       return -1;
5610     return devsw[ip->major].write(ip, src, n);
5611   }
5612 
5613   if(off > ip->size || off + n < off)
5614     return -1;
5615   if(off + n > MAXFILE*BSIZE)
5616     return -1;
5617 
5618   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5619     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5620     m = min(n - tot, BSIZE - off%BSIZE);
5621     memmove(bp->data + off%BSIZE, src, m);
5622     log_write(bp);
5623     brelse(bp);
5624   }
5625 
5626   if(n > 0 && off > ip->size){
5627     ip->size = off;
5628     iupdate(ip);
5629   }
5630   return n;
5631 }
5632 
5633 
5634 
5635 
5636 
5637 
5638 
5639 
5640 
5641 
5642 
5643 
5644 
5645 
5646 
5647 
5648 
5649 
5650 // Directories
5651 
5652 int
5653 namecmp(const char *s, const char *t)
5654 {
5655   return strncmp(s, t, DIRSIZ);
5656 }
5657 
5658 // Look for a directory entry in a directory.
5659 // If found, set *poff to byte offset of entry.
5660 struct inode*
5661 dirlookup(struct inode *dp, char *name, uint *poff)
5662 {
5663   uint off, inum;
5664   struct dirent de;
5665 
5666   if(dp->type != T_DIR)
5667     panic("dirlookup not DIR");
5668 
5669   for(off = 0; off < dp->size; off += sizeof(de)){
5670     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5671       panic("dirlink read");
5672     if(de.inum == 0)
5673       continue;
5674     if(namecmp(name, de.name) == 0){
5675       // entry matches path element
5676       if(poff)
5677         *poff = off;
5678       inum = de.inum;
5679       return iget(dp->dev, inum);
5680     }
5681   }
5682 
5683   return 0;
5684 }
5685 
5686 
5687 
5688 
5689 
5690 
5691 
5692 
5693 
5694 
5695 
5696 
5697 
5698 
5699 
5700 // Write a new directory entry (name, inum) into the directory dp.
5701 int
5702 dirlink(struct inode *dp, char *name, uint inum)
5703 {
5704   int off;
5705   struct dirent de;
5706   struct inode *ip;
5707 
5708   // Check that name is not present.
5709   if((ip = dirlookup(dp, name, 0)) != 0){
5710     iput(ip);
5711     return -1;
5712   }
5713 
5714   // Look for an empty dirent.
5715   for(off = 0; off < dp->size; off += sizeof(de)){
5716     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5717       panic("dirlink read");
5718     if(de.inum == 0)
5719       break;
5720   }
5721 
5722   strncpy(de.name, name, DIRSIZ);
5723   de.inum = inum;
5724   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5725     panic("dirlink");
5726 
5727   return 0;
5728 }
5729 
5730 
5731 
5732 
5733 
5734 
5735 
5736 
5737 
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
5750 // Paths
5751 
5752 // Copy the next path element from path into name.
5753 // Return a pointer to the element following the copied one.
5754 // The returned path has no leading slashes,
5755 // so the caller can check *path=='\0' to see if the name is the last one.
5756 // If no name to remove, return 0.
5757 //
5758 // Examples:
5759 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5760 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5761 //   skipelem("a", name) = "", setting name = "a"
5762 //   skipelem("", name) = skipelem("////", name) = 0
5763 //
5764 static char*
5765 skipelem(char *path, char *name)
5766 {
5767   char *s;
5768   int len;
5769 
5770   while(*path == '/')
5771     path++;
5772   if(*path == 0)
5773     return 0;
5774   s = path;
5775   while(*path != '/' && *path != 0)
5776     path++;
5777   len = path - s;
5778   if(len >= DIRSIZ)
5779     memmove(name, s, DIRSIZ);
5780   else {
5781     memmove(name, s, len);
5782     name[len] = 0;
5783   }
5784   while(*path == '/')
5785     path++;
5786   return path;
5787 }
5788 
5789 
5790 
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 // Look up and return the inode for a path name.
5801 // If parent != 0, return the inode for the parent and copy the final
5802 // path element into name, which must have room for DIRSIZ bytes.
5803 // Must be called inside a transaction since it calls iput().
5804 static struct inode*
5805 namex(char *path, int nameiparent, char *name)
5806 {
5807   struct inode *ip, *next;
5808 
5809   if(*path == '/')
5810     ip = iget(ROOTDEV, ROOTINO);
5811   else
5812     ip = idup(proc->cwd);
5813 
5814   while((path = skipelem(path, name)) != 0){
5815     ilock(ip);
5816     if(ip->type != T_DIR){
5817       iunlockput(ip);
5818       return 0;
5819     }
5820     if(nameiparent && *path == '\0'){
5821       // Stop one level early.
5822       iunlock(ip);
5823       return ip;
5824     }
5825     if((next = dirlookup(ip, name, 0)) == 0){
5826       iunlockput(ip);
5827       return 0;
5828     }
5829     iunlockput(ip);
5830     ip = next;
5831   }
5832   if(nameiparent){
5833     iput(ip);
5834     return 0;
5835   }
5836   return ip;
5837 }
5838 
5839 struct inode*
5840 namei(char *path)
5841 {
5842   char name[DIRSIZ];
5843   return namex(path, 0, name);
5844 }
5845 
5846 
5847 
5848 
5849 
5850 struct inode*
5851 nameiparent(char *path, char *name)
5852 {
5853   return namex(path, 1, name);
5854 }
5855 
5856 
5857 
5858 
5859 
5860 
5861 
5862 
5863 
5864 
5865 
5866 
5867 
5868 
5869 
5870 
5871 
5872 
5873 
5874 
5875 
5876 
5877 
5878 
5879 
5880 
5881 
5882 
5883 
5884 
5885 
5886 
5887 
5888 
5889 
5890 
5891 
5892 
5893 
5894 
5895 
5896 
5897 
5898 
5899 
