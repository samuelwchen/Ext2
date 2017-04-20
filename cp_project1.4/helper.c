#include "project.h"

int get_block(int fd, int blk, char buf[ ])
{
  debugMode("get_block()\n");
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}

int put_block(int dev, int blk, char buf[ ])
{
  lseek(dev, (long)blk*BLKSIZE, 0);
  write(dev, buf, BLKSIZE);
}


// minode_table is a global
void init (PROC proc[NPROC], MINODE *root)
{
  debugMode("init()\n");
  for (int i = 0; i < NMINODE; i++)
  {
    minode_table[i].dev = 0;
    minode_table[i].ino = 0;
    minode_table[i].refCount = 0;
    minode_table[i].dirty = 0;
    minode_table[i].mounted = 0;
    minode_table[i].mountPtr = 0;
  }

  for (int i = 0; i < NPROC; i++)
  {
    proc[i].pid = 0;    // this might get changed during fork
    proc[i].uid = i;
    proc[i].next = NULL;
    proc[i].cwd = NULL;
    //proc[i].fd = NULL;    // not sure if this will work.  should have *fd[NFD] eventually
    for (int j = 0; j < NFD; j++)
    {
      proc[i].fd[j] = NULL;
    }
  }

  root = NULL;
}
void mount_root(int dev, MINODE **root)
{
  debugMode("mount_root()\n");
  *root = iget(dev, 2);
}

int get_itable_begin(int dev, GD *gp)
{
  debugMode("get_itable_begin()\n");
  char buf[BLKSIZE];
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  return gp->bg_inode_table;
}




MINODE *iget(int dev, int ino)
{
  debugMode("iget()\n");
  int iblock, blk, disp;    // disp - where in blk is the inode located
  char buf[BLKSIZE];
  MINODE *mip;
  INODE *ip;

  GD *gp;

  iblock = get_itable_begin(dev, gp);

  // looking for matching minode in minode_table
  for (int i = 0; i < NMINODE; i++)
  {
    mip = &minode_table[i];
    if (mip->dev == dev && mip->ino == ino)
    {
       mip->refCount++;
       debugMode("Found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }

  // no match.  finding unused minode in minode_table
  for (int i = 0; i < NMINODE; i++)
  {
    mip = &minode_table[i];
    if (mip->refCount == 0){
       debugMode("Allocating NEW minode[%d] for [dev = %d, ino = %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;  // assing to (dev, ino)
       mip->dirty = 0;
       mip->mounted = 0;
       mip->mountPtr = 0;

       // get INODE of ino into buf[ ]
       blk  = (ino-1)/8 + iblock;  // iblock = Inodes start block #
       disp = (ino-1) % 8;

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;

       // copy INODE to mp->INODE
       mip->inode = *ip;
       return mip;
    }
  }
}

void iput(MINODE *mip)  // dispose of a minode[] pointed by mip
{
  debugMode("iput()\n");
  int iblock, blk, disp;    // disp - where in blk is the inode located
  char buf[BLKSIZE];
  INODE *ip;
  GD *gp;

  mip->refCount--;

  if (mip->refCount > 0)
    return;
  if (!mip->dirty)
    return;

  get_block(mip->dev,2, buf);
  gp = (GD*)buf;
  iblock = get_itable_begin(mip->dev, gp);

  /* write INODE back to disk */
  debugMode("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

   // get INODE of ino into buf[ ]
   blk  = (mip->ino - 1) / 8 + iblock;  // iblock = Inodes start block #
   disp = (mip->ino - 1) % 8;

   get_block(mip->dev, blk, buf);

    ip = (INODE *)buf + disp;
    *ip = mip->inode;

    put_block(mip->dev, blk, buf);
}

int getino(int dev,PROC *running, char pathname[DEPTH][NAMELEN])
{
  debugMode("getino()");
  int ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  // PRINTING PATH
  printf("getino(): pathname = ");
  for (int i =0; i < DEPTH && pathname[i][0] != '\0'; i++)
    printf("\"%s\" ", pathname[i]);
  printf("\n");

  if ( !strcmp(pathname[0], "/") && pathname[1][0] == '\0')
      return 2;

  if ( !strcmp(pathname[0], "/") )
     mip = iget(dev, 2);
  else
     mip = iget(running->cwd->dev, running->cwd->ino);

  for (int i=0; strcmp(pathname[i], "\0") != 0; i++)
  {
      if ( !strcmp(pathname[i], "/") )
        i++; //don't look for "/"
      printf("===========================================\n");
      printf("pathname[%d]=%s\n", i, pathname[i]);

      ino = search(mip->dev, mip, pathname[i]);

      if (ino==0)
      {
         iput(mip);  //we no longer need this minode
         printf("Directory \"%s\" does not exist.\n", pathname[i]);
         return 0;
      }
      iput(mip);  //we no longer need this minode
      mip = iget(dev, ino); //getting next inode
   }
   iput(mip);
   return ino;
}





//copied this over
void checkMagicNumber(int fd)
{
  debugMode("checkMagicNumber()\n");

  // read SUPER block
  char buf[BLKSIZE];
  get_block(fd, 1, buf);
  SUPER *sp = (SUPER *)buf;

  // check for EXT2 magic number:
  printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53)
  {
    printf("NOT an EXT2 FS\n");
    exit(1);
  }
  printf("It is an EXT2 FS.  WE ARE AWESOME!\n");
}

void printSuperBlock(int fd)
{
  debugMode("printSuperBlock()\n");
  char buf[BLKSIZE];
  get_block(fd, 1, buf);
  SUPER *sp = (SUPER *)buf;

  printf("=================== Super Block Contents ==================\n");
  printf("s_inodes_count \t\t\t\t\t%d\n", sp->s_inodes_count);
  printf("s_blocks_count \t\t\t\t\t%d\n", sp->s_blocks_count);
  printf("s_free_inodes_count \t\t\t\t%d\n", sp->s_free_inodes_count);
  printf("s_free_blocks_count \t\t\t\t%d\n", sp->s_free_blocks_count);
  printf("s_first_data_block \t\t\t\t%d\n", sp->s_first_data_block);
  printf("s_blocks_per_group \t\t\t\t%d\n", sp->s_blocks_per_group);
  printf("s_inodes_per_group \t\t\t\t%d\n", sp->s_inodes_per_group);
  printf("s_magic \t\t\t\t\t%x\n", (sp->s_magic));
  printf("\n");
}

void printInode(INODE* ipCur)
{
  debugMode("printInode()\n");
  printf("=================== Inode Contents ==================\n");
  //printf("i_mode \t\t\t\t\t%hu\n", ipCur->imode);
  printf("i_uid \t\t\t\t\t%d\n", ipCur->i_uid);
  printf("i_gid \t\t\t\t\t%d\n", ipCur->i_gid);
  printf("i_size (in bytes) \t\t\t%d\n", ipCur->i_size);
  printf("-------------- disc blocks in inode iblocks table --------------\n");
  for (int i = 0; i < 15; i++)
    printf("block[%d] = %d\n", i, ipCur->i_block[i]);
  printf("\n");
}

void fillItUp(int dev, PROC* running)
{
  char pathname[DEPTH][NAMELEN];
  strcpy (pathname[0], "/");
  strcpy (pathname[1], "X");

  char buf[NAMELEN] = { '\0 '};
  for (int i = 0; i < 52-9; i++)
  {
    strcpy(buf, "newdirectoryhere");
    if (i < 26)
      buf[12] = 'A'+i;
    else
      buf[12] = ('a'+i-26) ;
    buf[13] = '\0';
    strcpy(pathname[2], buf);
    _mkdir (dev, running, pathname);

  }




}

void addSingleEntryBlock(int dev, PROC* running)
{
  int ino = ialloc(running->cwd->dev);
  int bno = balloc(running->cwd->dev);

  if (bno == 0 | ino == 0)
  {
    printf("my_mkdir(): bno = %d, ino = %d", bno, ino);
    return;
  }

  //"ALLOCATE" MIP
  //point cwd at directory X
  char pathname[DEPTH][BLKSIZE];
  strcpy(pathname[0], "/");
  strcpy(pathname[1], "X");
  cd(dev,running, pathname);
  MINODE *pip = iget(dev, 12);
  MINODE *mip = iget(pip->dev, ino);

  //INIT CONTENTS OF MIP
  INODE *ip = &(mip->inode);
  ip->i_mode = 040755;      //DIR type and permissions
  ip->i_uid = running->uid; //user's id
  ip->i_gid = 0;            //Group id MAY NEED TO CHANGE
  ip->i_size = BLKSIZE;
  ip->i_links_count = 2;    //links for . and ..
  ip->i_atime = time(0L);   //set to current time
  ip->i_ctime = ip->i_atime;//set to current time
  ip->i_mtime = ip->i_atime;//set to current time
  ip->i_blocks = 2;         //Linux: blocks count in 512-bytes chunks
  ip->i_block[0] = bno;     //Will need for dir entries . and ..
  for (int i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;     //Unused blocks
  }
  mip->dirty = 1;           //mark dirty (so it will be written to mem)

  //WRITE MIP BACK TO MEMORY (DEALLOCATE IT)
  iput(mip);

  //INIT THE NEW DATABLOCK (I_BLOCK[0])
  char dp_buf[BLKSIZE];
  DIR *dp = (DIR *)dp_buf;
  char *cp = NULL;

  //INIT THE FIRST ENTRY "."
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  cp = (char *)(dp->name);
  *cp = '.';

  //INIT SECOND ENTRY ".."
  dp = (DIR *)( (char *)dp + dp->rec_len);
  dp->inode = pip->ino;
  dp->rec_len = BLKSIZE - 12;           //remaining space in block
  dp->name_len = 2;
  cp = (char *)(dp->name);
  *cp = '.';
  cp++;
  *cp = '.';

  //WRITE NEW DATABLOCK BACK TO MEMORY
  put_block(pip->dev, bno, dp_buf);

  bno = balloc(running->cwd->dev);
  //INIT THE NEW DATABLOCK (I_BLOCK[0])
  char new_dp_buf[BLKSIZE];
  dp = (DIR *)new_dp_buf;
  cp = NULL;

  //INIT THE FIRST ENTRY "."
  dp->inode = ino;
  dp->rec_len = 1024;
  dp->name_len = 1;
  cp = (char *)(dp->name);
  *cp = 'M';

  //WRITE NEW DATABLOCK BACK TO MEMORY
  put_block(pip->dev, bno, new_dp_buf);

  pip->inode.i_block[2] = pip->inode.i_block[1];
  pip->inode.i_block[1] = bno;
  iput(pip);
}
