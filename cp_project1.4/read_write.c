#include "project.h"


int _read(OFT *fd, char *buf, int nbytes)
{
  //CALCULATE AVIALABLE BLOCKS
  int avil = fd->mptr->inode.i_size - fd->offset;
  int lbk = 0;
  int start = 0;
  int totalBytes = 0;
  int remaining = 0;
  int bno = 0;
  char dbuf[BLKSIZE] = {'\0'};
  char *cp_dbuf = dbuf;
  char *cp_buf = buf;

  //WHILE THERE ARE STILL BYTES TO READ
  while (nbytes > 0 && avil > 0)
  {
    //CALCULATE LOGICAL BLOCK NUMBER AND THE START BYTE
    int lbk = fd->offset / BLKSIZE;
    int start = fd->offset % BLKSIZE;

    //GET BNO FROM LBK
    bno = bnoFromOffset(fd, lbk);

    //GET DATABLOCK
    get_block(fd->mptr->dev, bno, dbuf);

    //CALCULATE REMAINING BYTES IN BLOCK AND GET START POINTER
    remaining = BLKSIZE - start;
    avil = fd->mptr->inode.i_size - fd->offset;

    //CHECK WHETHER NBYTES, AVIL, OR REMAINING NEED TO BE READ INTO BUF
    if (nbytes < remaining && nbytes <= avil)
      remaining = nbytes;
    else if (avil < remaining && avil < nbytes)
      remaining = nbytes;

    //COPY (ACTUAL) REMAINING BYTES INTO BUF
    cp_dbuf = dbuf + start;  //set start pointer
    memcpy(buf, cp_dbuf, remaining);

    debugMode("small check: ");
    for(int i = 0; i < 10; i++)
    {
      debugMode("%c", buf[i]);
    }debugMode("\n");

    //UPDATE NBYTES, AVIL, OFFSET, CP_BUF, AND TOTALBYTES
    fd->offset += remaining;
    avil -= remaining;
    nbytes -= remaining;
    totalBytes += remaining;
  }
  return totalBytes;
}

/* returns the bno in i_block[] from the offset in the file*/
int bnoFromOffset(OFT *fd, int lbk)
{
  int *indirectPtr = NULL;
  int *doubleIndirectPtr = NULL;
  char buf[BLKSIZE] = {'\0'};
  if(lbk < 12)
  {
    printf("NOTE: lbk < 12\n");
    return fd->mptr->inode.i_block[lbk];
  }
  else if(lbk >= 12 && lbk < (256 + 12))
  {
    printf("NOTE: 12 <= lbk < 256\n");
    //GET INDIRECT BLOCK
    get_block(fd->mptr->dev, fd->mptr->inode.i_block[12], buf);

    //FIND INDIRECT BLOCK FROM LBK
    indirectPtr = (int *)buf + (lbk - 12);

    debugMode("Buf = %d, ptr = %d\n", buf, indirectPtr);

    return *indirectPtr;
  }
  else
  {
    printf("NOTE: 256 <= lbk < ((256*265) + 256 + 12)\n");
    int level_1_i = (lbk - (256 + 12)) / 256;
    int level_2_i = (lbk - (256 + 12)) % 256;
    debugMode("level1 = %d, level2 = %d\n", level_1_i, level_2_i);
    //GET DOUBLE INDIRECT BLOCK
    get_block(fd->mptr->dev, fd->mptr->inode.i_block[13], buf);
    doubleIndirectPtr = (int *)buf + level_1_i;

    debugMode("First block: buf = %d, ptr1 = %d\n", buf, doubleIndirectPtr);

    get_block(fd->mptr->dev, *doubleIndirectPtr, buf);
    doubleIndirectPtr = (int *)buf + level_2_i;

    debugMode("Second block: buf = %d, ptr2 = %d\n", buf, doubleIndirectPtr);

    return *doubleIndirectPtr;
  }
}
