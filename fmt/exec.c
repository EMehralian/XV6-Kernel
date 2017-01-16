6650 #include "types.h"
6651 #include "param.h"
6652 #include "memlayout.h"
6653 #include "mmu.h"
6654 #include "proc.h"
6655 #include "defs.h"
6656 #include "x86.h"
6657 #include "elf.h"
6658 
6659 int
6660 exec(char *path, char **argv)
6661 {
6662   char *s, *last;
6663   int i, off;
6664   uint argc, sz, sp, ustack[3+MAXARG+1];
6665   struct elfhdr elf;
6666   struct inode *ip;
6667   struct proghdr ph;
6668   pde_t *pgdir, *oldpgdir;
6669 
6670   begin_op();
6671 
6672   if((ip = namei(path)) == 0){
6673     end_op();
6674     return -1;
6675   }
6676   ilock(ip);
6677   pgdir = 0;
6678 
6679   // Check ELF header
6680   if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
6681     goto bad;
6682   if(elf.magic != ELF_MAGIC)
6683     goto bad;
6684 
6685   if((pgdir = setupkvm()) == 0)
6686     goto bad;
6687 
6688   // Load program into memory.
6689   sz = 0;
6690   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6691     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6692       goto bad;
6693     if(ph.type != ELF_PROG_LOAD)
6694       continue;
6695     if(ph.memsz < ph.filesz)
6696       goto bad;
6697     if(ph.vaddr + ph.memsz < ph.vaddr)
6698       goto bad;
6699     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
6700       goto bad;
6701     if(ph.vaddr % PGSIZE != 0)
6702       goto bad;
6703     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
6704       goto bad;
6705   }
6706   iunlockput(ip);
6707   end_op();
6708   ip = 0;
6709 
6710   // Allocate two pages at the next page boundary.
6711   // Make the first inaccessible.  Use the second as the user stack.
6712   sz = PGROUNDUP(sz);
6713   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6714     goto bad;
6715   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6716   sp = sz;
6717 
6718   // Push argument strings, prepare rest of stack in ustack.
6719   for(argc = 0; argv[argc]; argc++) {
6720     if(argc >= MAXARG)
6721       goto bad;
6722     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6723     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6724       goto bad;
6725     ustack[3+argc] = sp;
6726   }
6727   ustack[3+argc] = 0;
6728 
6729   ustack[0] = 0xffffffff;  // fake return PC
6730   ustack[1] = argc;
6731   ustack[2] = sp - (argc+1)*4;  // argv pointer
6732 
6733   sp -= (3+argc+1) * 4;
6734   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6735     goto bad;
6736 
6737   // Save program name for debugging.
6738   for(last=s=path; *s; s++)
6739     if(*s == '/')
6740       last = s+1;
6741   safestrcpy(proc->name, last, sizeof(proc->name));
6742 
6743   // Commit to the user image.
6744   oldpgdir = proc->pgdir;
6745   proc->pgdir = pgdir;
6746   proc->sz = sz;
6747   proc->tf->eip = elf.entry;  // main
6748   proc->tf->esp = sp;
6749   switchuvm(proc);
6750   freevm(oldpgdir);
6751   return 0;
6752 
6753  bad:
6754   if(pgdir)
6755     freevm(pgdir);
6756   if(ip){
6757     iunlockput(ip);
6758     end_op();
6759   }
6760   return -1;
6761 }
6762 
6763 
6764 
6765 
6766 
6767 
6768 
6769 
6770 
6771 
6772 
6773 
6774 
6775 
6776 
6777 
6778 
6779 
6780 
6781 
6782 
6783 
6784 
6785 
6786 
6787 
6788 
6789 
6790 
6791 
6792 
6793 
6794 
6795 
6796 
6797 
6798 
6799 
