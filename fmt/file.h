4150 struct file {
4151   enum { FD_NONE, FD_PIPE, FD_INODE } type;
4152   int ref; // reference count
4153   char readable;
4154   char writable;
4155   struct pipe *pipe;
4156   struct inode *ip;
4157   uint off;
4158 };
4159 
4160 
4161 // in-memory copy of an inode
4162 struct inode {
4163   uint dev;           // Device number
4164   uint inum;          // Inode number
4165   int ref;            // Reference count
4166   struct sleeplock lock;
4167   int flags;          // I_VALID
4168 
4169   short type;         // copy of disk inode
4170   short major;
4171   short minor;
4172   short nlink;
4173   uint size;
4174   uint addrs[NDIRECT+1];
4175 };
4176 #define I_VALID 0x2
4177 
4178 // table mapping major device number to
4179 // device functions
4180 struct devsw {
4181   int (*read)(struct inode*, char*, int);
4182   int (*write)(struct inode*, char*, int);
4183 };
4184 
4185 extern struct devsw devsw[];
4186 
4187 #define CONSOLE 1
4188 
4189 
4190 
4191 
4192 
4193 
4194 
4195 
4196 
4197 
4198 
4199 
4200 // Blank page.
4201 
4202 
4203 
4204 
4205 
4206 
4207 
4208 
4209 
4210 
4211 
4212 
4213 
4214 
4215 
4216 
4217 
4218 
4219 
4220 
4221 
4222 
4223 
4224 
4225 
4226 
4227 
4228 
4229 
4230 
4231 
4232 
4233 
4234 
4235 
4236 
4237 
4238 
4239 
4240 
4241 
4242 
4243 
4244 
4245 
4246 
4247 
4248 
4249 
