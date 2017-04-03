#include "project.h"

// "At this block, is it occupied?""
int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);
  printf("free blocks in super = %d\n", sp->s_free_blocks_count);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
  printf("free blocks in gd = %d\n", gp->bg_free_blocks_count);
}

int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);
  printf("free blocks in super = %d\n", sp->s_free_blocks_count);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
  printf("free blocks in gd = %d\n", gp->bg_free_blocks_count);
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);
  printf("free blocks in super = %d\n", sp->s_free_inodes_count);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
  printf("free blocks in gd = %d\n", gp->bg_free_inodes_count);
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];

  // dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);
  printf("free blocks in super = %d\n", sp->s_free_inodes_count);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
  printf("free blocks in gd = %d\n", gp->bg_free_inodes_count);
}
/**************************************************
Precondition :: correct dev
Info :: Allocates a datablock and returns bno number.
Returns 0 on failure.
**************************************************/
int balloc(int dev)
{

  char buf[BLKSIZE] = {'\0'};

  // READ SUPER BLOCK
  char sp_buf[BLKSIZE] = {'\0'};
  get_block(dev, 1, sp_buf);
  SUPER *sp = (SUPER *)sp_buf;

  //GET NUMBER BLOCKS
  int ninodes = sp->s_inodes_count;
  int nblocks = sp->s_blocks_count;
  int nfreeInodes = sp->s_free_inodes_count;
  int nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
  ninodes, nblocks, nfreeInodes, nfreeBlocks);

  // READ GROUP DESCIPTOR
  GD *gp = NULL;
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  // GET BMAP BLOCK NUMBER
  int bmap = gp->bg_block_bitmap;
  printf("bmap = %d\n", bmap);

  // READ inode_bitmap BLOCK
  get_block(dev, bmap, buf);

  //CHECK FOR FREE BLOCK
  for (int i=0; i < nblocks; i++)
  {
    if (tst_bit(buf, i)==0)
    {
      //SET BIT AND RETURN BLOCK NUMBER
       set_bit(buf,i);
       decFreeBlocks(dev);

       put_block(dev, bmap, buf);

       return i+1;
    }
  }
  printf("balloc(): no more free blocks\n");
  return 0;
}

/**************************************************
Precondition :: correct dev
Info :: Allocates a inode block and returns ino number.
Returns 0 on failure.
**************************************************/
//YOU ARE HERE
int ialloc(int dev)
{

  char buf[BLKSIZE] = {'\0'};

  // READ SUPER BLOCK
  char sp_buf[BLKSIZE] = {'\0'};
  get_block(dev, 1, sp_buf);
  SUPER *sp = (SUPER *)sp_buf;

  //GET NUMBER INODES
  int ninodes = sp->s_inodes_count;
  int nblocks = sp->s_blocks_count;
  int nfreeInodes = sp->s_free_inodes_count;
  int nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
  ninodes, nblocks, nfreeInodes, nfreeBlocks);

  // READ GROUP DESCIPTOR
  GD *gp = NULL;
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  // GET IMAP BLOCK NUMBER
  int imap = gp->bg_inode_bitmap;
  printf("bmap = %d\n", imap);

  // READ inode_bitmap BLOCK
  get_block(dev, imap, buf);

  //CHECK FOR FREE BLOCK
  for (int i=0; i < ninodes; i++)
  {
    if (tst_bit(buf, i)==0)
    {
      //SET BIT AND RETURN BLOCK NUMBER
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}
/**************************************************
Precondition ::
Info :: clears the bit mapped to the block number
in the bmap
**************************************************/
int bdealloc(int dev, int bno)
{
  char buf[BLKSIZE] = {'\0'};

  // READ SUPER BLOCK
  char sp_buf[BLKSIZE] = {'\0'};
  get_block(dev, 1, sp_buf);
  SUPER *sp = (SUPER *)sp_buf;

  //GET NUMBER INODES
  int ninodes = sp->s_inodes_count;
  int nblocks = sp->s_blocks_count;
  int nfreeInodes = sp->s_free_inodes_count;
  int nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
  ninodes, nblocks, nfreeInodes, nfreeBlocks);

  // READ GROUP DESCIPTOR
  GD *gp = NULL;
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  // GET IMAP BLOCK NUMBER
  int bmap = gp->bg_block_bitmap;
  printf("bmap = %d\n", bmap);

  // READ inode_bitmap BLOCK
  get_block(dev, bmap, buf);
  //CLEAR BIT IN BUF CONTAINING IMAP
  clr_bit(buf, bno - 1);
  // WRITE BUF BACK TO IMAP ON DEV
  put_block(dev, bmap, buf);
  incFreeBlocks(dev);
  return 1;
}
/**************************************************
Precondition ::
Info :: clears the bit mapped to the inode number
in the imap
**************************************************/
int idealloc(int dev, int ino)
{
  char buf[BLKSIZE] = {'\0'};

  // READ SUPER BLOCK
  char sp_buf[BLKSIZE] = {'\0'};
  get_block(dev, 1, sp_buf);
  SUPER *sp = (SUPER *)sp_buf;

  //GET NUMBER INODES
  int ninodes = sp->s_inodes_count;
  int nblocks = sp->s_blocks_count;
  int nfreeInodes = sp->s_free_inodes_count;
  int nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n",
  ninodes, nblocks, nfreeInodes, nfreeBlocks);

  // READ GROUP DESCIPTOR
  GD *gp = NULL;
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  // GET IMAP BLOCK NUMBER
  int imap = gp->bg_inode_bitmap;
  printf("imap = %d\n", imap);

  // READ inode_bitmap BLOCK
  get_block(dev, imap, buf);
  //CLEAR BIT IN BUF CONTAINING IMAP
  clr_bit(buf, ino - 1);
  // WRITE BUF BACK TO IMAP ON DEV
  put_block(dev, imap, buf);
  incFreeInodes(dev);
  return 1;
}
