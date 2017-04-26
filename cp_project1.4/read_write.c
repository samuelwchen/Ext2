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
      remaining = avil;

    //COPY (ACTUAL) REMAINING BYTES INTO BUF
    cp_dbuf = dbuf + start;  //set start pointer
    memcpy(buf, cp_dbuf, remaining);

    // debugMode("small check: ");
    // for(int i = 0; i < 10; i++)
    // {
    //   debugMode("%c", buf[i]);
    // }debugMode("\n");

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
    debugMode("NOTE: lbk < 12\n");
    return fd->mptr->inode.i_block[lbk];
  }
  else if(lbk >= 12 && lbk < (256 + 12))
  {
    debugMode("NOTE: 12 <= lbk < 256\n");
    //GET INDIRECT BLOCK
    get_block(fd->mptr->dev, fd->mptr->inode.i_block[12], buf);

    //FIND INDIRECT BLOCK FROM LBK
    indirectPtr = (int *)buf + (lbk - 12);

    //debugMode("Buf = %d, ptr = %d\n", buf, indirectPtr);

    return *indirectPtr;
  }
  else
  {
    debugMode("NOTE: 256 <= lbk < ((256*265) + 256 + 12)\n");
    int level_1_i = (lbk - (256 + 12)) / 256;
    int level_2_i = (lbk - (256 + 12)) % 256;
    //debugMode("level1 = %d, level2 = %d\n", level_1_i, level_2_i);
    //GET DOUBLE INDIRECT BLOCK
    get_block(fd->mptr->dev, fd->mptr->inode.i_block[13], buf);
    doubleIndirectPtr = (int *)buf + level_1_i;

    //debugMode("First block: buf = %d, ptr1 = %d\n", buf, doubleIndirectPtr);

    get_block(fd->mptr->dev, *doubleIndirectPtr, buf);
    doubleIndirectPtr = (int *)buf + level_2_i;

    //debugMode("Second block: buf = %d, ptr2 = %d\n", buf, doubleIndirectPtr);

    return *doubleIndirectPtr;
  }
}

void printRead(PROC *running, int fd_num, int nBytes)
{
  OFT *oftp = &( running->fd[fd_num] );
  //CHECK FD_num is in range
  if(fd_num < 0 || fd_num >= NFD)
  {
    printf("File descriptor number out of bounds. Aborting close.\n");
    return;
  }
  //CHECK FD[FD_NUM] HAS A VALID FD
  if (oftp->refCount <= 0)
  {
    printf("Invalid file descriptor chosen. File descriptor %d chosen. Aborting close.\n", fd_num);
    return;
  }
  if (oftp->mode != 0 && oftp->mode != 2)
  {
    printf("File in incorrect mode.  Must be r or rw mode.\n");
    return;
  }
  //CREATE BUF TO PRINT TO SCREEN
  if (nBytes > oftp->mptr->inode.i_size)
    nBytes = oftp->mptr->inode.i_size;
  if (nBytes > MAXINT)
    nBytes = MAXINT - 1;


  //char buf[nBytes + 1];
  char buf[BLKSIZE+1];
  int bytesRead = 0;

//  for (int i = 0; i < (nBytes + 1); i++)
  for (int i = 0; i < BLKSIZE+1; i++)
  {
    buf[i] = '\0';
  }


  while (nBytes > 0)
  {
    int bytesReadInBlk = 0;
    if (nBytes < BLKSIZE)
    {
      bytesReadInBlk = _read(oftp, buf, nBytes);
      bytesRead += bytesReadInBlk;
      nBytes -= bytesReadInBlk;
    }
    else
    {
      bytesReadInBlk = _read(oftp, buf, BLKSIZE);
      bytesRead += bytesReadInBlk;
      nBytes -= bytesReadInBlk;
    }
    printf("%s", buf);

  }

  printf("\n--------------------------------------------\n");
  printf("Bytes read = %d\n", bytesRead);
  return;
}


void _lseek(PROC *running, int fd_num, int position)
{
  // check if valid fd_index
  if(fd_num < 0 || fd_num >= NFD)
  {
    printf("File descriptor number out of bounds. Aborting lseek.\n");
    return;
  }
  if (running->fd[fd_num].refCount <= 0)
  {
    printf("Invalid file descriptor chosen. File descriptor %d chosen. Aborting lseek.\n", fd_num);
    return;
  }

  // CHECKING IF POSITION IS OUT OF RANGE
  if (position > running->fd[fd_num].mptr->inode.i_size)
    position = running->fd[fd_num].mptr->inode.i_size;

  running->fd[fd_num].offset = position;

  printf("-------------------------------------------\n");
  printf("lseek placed at position %d of file.\n", position);
  return;
}

int _write(OFT *fd, char *buf, int nbytes)
{
  //CALCULATE AVIALABLE BLOCKS
  //int avil = fd->mptr->inode.i_size - fd->offset;
  int lbk = 0;
  int start = 0;
  int totalBytes = 0;
  int remaining = 0;
  int bno = 0;
  char file_buf[BLKSIZE] = {'\0'};
  char *cp_file_buf = file_buf;
  char *cp_buf = buf;

  //WHILE THERE ARE STILL BYTES TO WRITE
  //while (nbytes > 0 && avil > 0)
  while (nbytes > 0 )
  {
    //CALCULATE LOGICAL BLOCK NUMBER AND THE START BYTE
    int lbk = fd->offset / BLKSIZE;
    int start = fd->offset % BLKSIZE;

    //GET BNO FROM LBK
    bno = bnoFromOffset(fd, lbk);

    //CHECK IF NEED TO ALLOCATE BLOCK
    if (bno == 0)
    {
      bno = balloc(fd->mptr->dev);
      if(bno == 0)
      {
        printf("No more datablocks avialable.  Aborting write\n");
        return totalBytes;
      }
    }

    //GET DATABLOCK
    get_block(fd->mptr->dev, bno, file_buf);

    //CALCULATE REMAINING BYTES IN BLOCK AND GET START POINTER
    remaining = BLKSIZE - start;
    //avil = fd->mptr->inode.i_size - fd->offset;

    //CHECK WHETHER NBYTES OR REMAINING NEED TO BE READ OUT OF BUF into file_buf
    if (nbytes < remaining )
      remaining = nbytes;
    // else if (avil < remaining && avil < nbytes)
    //   remaining = avil;

    //COPY (ACTUAL) REMAINING BYTES INTO file
    cp_file_buf = file_buf + start;  //set start pointer
    memcpy( cp_file_buf, cp_buf, remaining);

    //UPDATE NBYTES, OFFSET, CP_BUF, AND TOTALBYTES
    put_block(fd->mptr->dev, bno, file_buf);
    fd->offset += remaining;
    //avil -= remaining;
    nbytes -= remaining;
    totalBytes += remaining;
    cp_buf += remaining;
    if(fd->offset >= fd->mptr->inode.i_size)
      fd->mptr->inode.i_size = fd->offset;  //offet has already been updated
  }
  return totalBytes;
}

void screen_write(PROC *running, int fd_num, char *buf)
{
  OFT *oftp = &( running->fd[fd_num] );
  //CHECK FD_num is in range
  if(fd_num < 0 || fd_num >= NFD)
  {
    printf("File descriptor number out of bounds. Aborting close.\n");
    return;
  }
  //CHECK FD[FD_NUM] HAS A VALID FD
  if (oftp->refCount <= 0)
  {
    printf("Invalid file descriptor chosen. File descriptor %d chosen. Aborting close.\n", fd_num);
    return;
  }
  if (oftp->mode == 0)
  {
    printf("File in incorrect mode.  Must be w,rw, or a mode.\n");
    return;
  }
  char *cp_buf = buf;


  _write(oftp, buf, strlen(buf));
}
