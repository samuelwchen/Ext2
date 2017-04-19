#include "project.h"

/******************************************
precondition: None
info: Removes a directory after checking to see if
directory is empty.
******************************************/
void rmDir (int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{

  MINODE* mip = pathnameToMip(dev, running, pathname[DEPTH][NAMELEN]);
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

  if (mip->inode.i_links_count)
  {
    printf("Directory is referenced by more than 2 links count.  Currently referenced by %d links.  Cannot delete.\n", mip->inode.i_links_count);
    return;
  }

  if (mip->refCount != 1)
  {
    printf("Directory being used by %d other programs.  Cannot delete.\n", mip->refCount - 1);
    return;
  }

  // DELETION NOW POSSIBLE.  PREPARING TO DELETE
  int pino = search(dev, mip, "..");    // need parent directory
  MINODE *pmip = iget(dev, pino);
  DIR *prevdp = NULL;
  DIR *dp = NULL;
  char buf[BLKSIZE];


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
        if (((char *)dp + dp->rec_len > buf + BLKSIZE) && prevdp != NULL)   // target dir is the last entry in that particular datablock AND there are other records in datablock
        {
          rmEndFile(dev, dp, prevdp);
          return;
        }
        else if (((char *)dp + dp->rec_len > buf + BLKSIZE) && prevdp == NULL) // target dir is the ONLY entry.  Must find last block to replace it.
        {
          rmOnlyFile(dev, pmip, pmip->inode.i_block[i]);
          return;
        }
        else
        {
          rmMiddleFile(dev, dp, buf);
          return;
        }
      }
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


void rmEndFile(int dev, DIR* dp, DIR* prevdp)
{
  prevdp->rec_len = prevdp->rec_len + dp->rec_len;
}
void rmOnlyFile(int dev, MINODE *pmip, int *iblockToChange)
{

  int *lastValidBlock = NULL;

  for (int i = 0; i < 12; i++)
  {
    pmip->inode.i_block[i];
    if (pmip->inode.i_block[i] != 0)
      lastValidBlock = pmip->inode.i_block[i];
    else
      break;
  }
  if (pmip->inode.i_block[12])      // if non-zero value in iblock, check the indirect blocks
    findLastIblock(dev, 1, pmip->inode.i_block[12], lastValidBlock);
  if (pmip->inode.i_block[13])
    findLastIblock(dev, 2, pmip->inode.i_block[13], lastValidBlock);
  if (pmip->inode.i_block[14])
    findLastIblock(dev, 3, pmip->inode.i_block[14], lastValidBlock);

  if (*lastValidBlock == *iblockToChange)   // the block to be deleted is the last block
    iblockToChange = 0;
  else                                      // iblock to be set to 0 is replaced by last valid iblock
  {
    *iblockToChange = *lastValidBlock;
    *lastValidBlock = 0;
  }
}
void rmMiddleFile(int dev, DIR *dp, char buf[BLKSIZE])
{
  int deleted_rec_len = dp->rec_len;
  int n = BLKSIZE - ((char*)dp-(char*)buf) - dp->rec_len;
  memcpy(dp, (char*)dp + dp->rec_len, n);

  while ((char*)dp + dp->rec_len + deleted_rec_len < buf + BLKSIZE)
  {
    dp = (char*)dp + dp->rec_len;
  }
  dp->rec_len = dp->rec_len + deleted_rec_len;

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
