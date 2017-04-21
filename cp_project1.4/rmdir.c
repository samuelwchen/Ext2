#include "project.h"

/******************************************
precondition: None
info: Removes a directory after checking to see if
directory is empty.
******************************************/
void rmDir (int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{

  MINODE* mip = pathnameToMip(dev, running, pathname);
  if (mip == NULL)
    return;

  // mip is what i'm trying to delete.

  // CHECKING IF DIRECTORY IS VALID FOR DELETION
  if (mip == 0)
  {
    printf("No such directory.\n");
    return;
  }

  if (! S_ISDIR(mip->inode.i_mode))
  {
    printf("Not a directory.  Cannot delete.\n");
    return;
  }

  // check if directory is empty
  if (!isDirEmpty(dev, mip))
  {
    printf("Directory is not empty.  Cannot delete.\n");
    return;
  }

  if (mip->inode.i_links_count > 2)
  {
    printf("Directory is referenced by more than 2 links count.  Currently referenced by %d links.  Cannot delete.\n", mip->inode.i_links_count);
    return;
  }

  debugMode("refCount = %d", mip->refCount);
  if (mip->refCount != 1)
  {
    printf("Directory being used by %d other programs.  Cannot delete.\n", mip->refCount - 1);
    return;
  }

  // DELETION NOW POSSIBLE.  PREPARING TO DELETE
  int pino = search(dev, mip, "..");    // need parent directory
  MINODE *pmip = iget(dev, pino);
  // pmip->inode.i_links_count--;
  // pmip->dirty = 1;
  mip->inode.i_links_count--;
  mip->dirty = 1;

  rmDirEntry(dev, pmip, mip);
  iput(pmip);
  iput(mip);
}

void freeBlockHelper(int dev, int level_indirection, int block_num)
{
  char buf[BLKSIZE];
  get_block(dev, block_num, buf);
  int *pIndirect_blk = (int*)buf;

  if (level_indirection == 1)
  {
    for (int i = 0; i < BLKSIZE/sizeof(int); i++, pIndirect_blk++)
    {
      if (*pIndirect_blk != 0)
        bdealloc(dev, *pIndirect_blk);
      else
        return;
    }
  }
  else if (level_indirection > 1)
  {
    for (int i = 0; i < BLKSIZE/sizeof(int); i++, pIndirect_blk++)
    {
      if (*pIndirect_blk != 0)
        freeBlockHelper(dev, level_indirection - 1, *pIndirect_blk);
      else
        return;
    }
  }
  else
  {
    printf("freeBlockHelper reached level of indirection = 0.  Should NOT have happened.\n");
    return;
  }

}
void rmDirEntry(int dev, MINODE* pmip, MINODE* mip)
{
  DIR *prevdp = NULL;
  DIR *dp = NULL;
  char buf[BLKSIZE];

  // last link to be disconnected.  free inode and block_num
  if (mip->inode.i_links_count == 1)
  {
    for (int i = 0; i < 12; i++)
    {
      if (mip->inode.i_block[i] != 0)
        bdealloc(dev, mip->inode.i_block[i]);
      else
        break;
    }

    for (int i = 12; i < 15; i++)
    {
      if (mip->inode.i_block[i] == 0)
        break;

      freeBlockHelper(dev, i-11, mip->inode.i_block[i]);
    }
  }


  for (int i = 0; i < 12; i++)
  {
    prevdp = NULL;
    get_block(dev, pmip->inode.i_block[i], buf);
    dp = (DIR*)buf;
    while (dp < (DIR*)(buf + BLKSIZE))
    {
      if (mip->ino == dp->inode)    // target dir to delete found!
      {
        // THREE CASES FOR DIRECTORY DELETION
        if (((char *)dp + dp->rec_len >= buf + BLKSIZE) && prevdp != NULL)   // target dir is the last entry in that particular datablock AND there are other records in datablock
        {
          rmEndFile(dev, dp, prevdp, pmip->inode.i_block[i], buf);
          iput(pmip);
          iput(mip);
          return;
        }
        else if (((char *)dp + dp->rec_len >= buf + BLKSIZE) && prevdp == NULL) // target dir is the ONLY entry.  Must find last block to replace it.
        {
          rmOnlyFile(dev, pmip, &(pmip->inode.i_block[i]));
          iput(pmip);
          iput(mip);
          return;
        }
        else
        {
          rmMiddleFile(dev, dp, pmip->inode.i_block[i], buf);
          iput(pmip);
          iput(mip);
          return;
        }
      }
      prevdp = dp;
      dp = (DIR*)((char*)dp + dp->rec_len);
    }
  }


  // NOT IN DIRECT BLOCKS.  LOOKING AT LEVEL OF INDIRECTION
  if (pmip->inode.i_block[12])
  {

  }
}
/***************************************************************************
precondition: None
info: Called by remove dir.  Checks if directory empty by seeing if
i_block[1] == 0 and if i_block[0] contains only 2 entries, "." and ".."
***************************************************************************/
int isDirEmpty(int dev, MINODE* mip)
{
  char buf[STRLEN] = {'\0'};
  get_block(dev, mip->inode.i_block[0], buf);
  DIR *dp = (DIR *) buf;
  dp = (DIR *)((char *)dp + dp->rec_len);   // advance to the ".."

  if ((char*) dp + dp->rec_len < buf + BLKSIZE)
    return 0;

  if(mip->inode.i_block[1])
    return 0;

  return 1;
}


void rmEndFile(int dev, DIR* dp, DIR* prevdp, int block_num, char buf[BLKSIZE])
{
  idealloc(dev, dp->inode);
  prevdp->rec_len = prevdp->rec_len + dp->rec_len;
  put_block(dev, block_num, buf);
}
void rmOnlyFile(int dev, MINODE *pmip, int *iblockToChange)
{

  int *lastValidBlock = NULL;

  for (int i = 0; i < 12; i++)
  {
    if (pmip->inode.i_block[i] != 0)
      lastValidBlock = &(pmip->inode.i_block[i]);
    else
      break;
  }

  for (int i = 12; i < 15; i++)
  {
    if (pmip->inode.i_block[i] == 0)
      break;

    findLastIblock(dev, i-11, pmip->inode.i_block[i], lastValidBlock);
  }

  // CLEANING UP INODES AND BNO
  char buf[BLKSIZE];
  get_block(dev, *iblockToChange, buf);
  DIR *dp = (DIR*)buf;
  idealloc(dev, dp->inode);
  bdealloc(dev, *iblockToChange);

  *iblockToChange = *lastValidBlock;
  *lastValidBlock = 0;
}
//void rmMiddleFile(int dev, DIR *dp, char buf[BLKSIZE])
void rmMiddleFile(int dev, DIR *dp, int block_num, char buf[BLKSIZE])
{
  idealloc(dev, dp->inode);

  int deleted_rec_len = dp->rec_len;
  int n = BLKSIZE - ((char*)dp-(char*)buf) - dp->rec_len;
  memmove(dp, (char*)dp + dp->rec_len, n);

  while ((char*)dp + dp->rec_len + deleted_rec_len < buf + BLKSIZE)
  {
    dp = (DIR*)((char*)dp + dp->rec_len);
  }
  dp->rec_len = dp->rec_len + deleted_rec_len;
  put_block(dev, block_num, buf);
}
/***************************************************************************
precondition: None
info: rmOnlyFile.  Looks for last occupied block.  a POINTER is passed in to
point to last occupied block.
***************************************************************************/
void findLastIblock(int dev, int level_indirection, int block_num, int* lastValidBlock)
{
  char buf[BLKSIZE];
  get_block(dev, block_num, buf);
  int *pIndirect_blk = (int*)buf;

  if (level_indirection == 1)
  {
    for (int i = 0; i < BLKSIZE/sizeof(int); i++, pIndirect_blk++)
    {
      if (*pIndirect_blk != 0)
        lastValidBlock = pIndirect_blk;
      else
        return;
    }
  }
  else if (level_indirection > 1)
  {
    for (int i = 0; i < BLKSIZE/sizeof(int); i++, pIndirect_blk++)
    {
      if (*pIndirect_blk != 0)
        findLastIblock(dev, level_indirection - 1, *pIndirect_blk, lastValidBlock);
      else
        return;
    }
  }
  else
  {
    printf("rmdir reached level of indirection = 0.  Should NOT have happened.\n");
    return;
  }

}
