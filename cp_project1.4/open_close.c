#include "project.h"

/*
* deallocates all the datablocks
* DOES NOT IPUT(mip)
*/
void _truncate(MINODE *mip)
{
  char buf[BLKSIZE];
  //goes through each datablock in mip and deallocates them
  if (mip->inode.i_links_count == 1)
  {
    // RELEASE THEM ALL
    for (int i = 0; i < 12; i++)
    {
      if (mip->inode.i_block[i] != 0)
      {
        bdealloc(mip->dev, mip->inode.i_block[i]);
        mip->inode.i_block[i] = 0;
      }
      else
        break;
    }

    for (int i = 12; i < 15; i++)
    {
      if (mip->inode.i_block[i] == 0)
        break;

      freeBlockHelper(mip->dev, i-11, mip->inode.i_block[i]);
      mip->inode.i_block[i] = 0;
    }
  }
  else
  {
    return;
  }
  // MARK TIME
  mip->inode.i_atime = time(0L);
  mip->inode.i_mtime = time(0L);
  // SET FILESIZE TO 0
  mip->inode.i_size = 0;
  // MARK DIRTY
  mip->dirty = 1;
  return;
}

/**********************************************************************
info:  if SUCCESSFUL, returns the fd index.  if FAILURE, returns -1
***********************************************************************/
int open_file(int dev, PROC *running, char pathname[DEPTH][NAMELEN], char mode[BLKSIZE])
{
  //CONVERT MODE FROM STR TO INT
  int modeNum = determineMode(mode);
  if (modeNum == -1)
  {
    printf("Invalid mode option.  Aborting open.\n");
    return -1;
  }

  //GET INO THEN MIP OF PATHNAME
  int ino =  getino(dev, running, pathname);
  if (ino == 0)
  {
    printf("Invalid file.  Aborting open.\n");
    return -1;
  }
  MINODE *mip = iget(dev, ino);

  //CHECK IF REG FILE (IF NOT, ABORT)
  if(!S_ISREG(mip->inode.i_mode))
  {
    printf("Not a regular file.  Aborting open.\n");
    iput(mip);
    return -1;
  }

  //CHECK IF IN USE OTHER THAN READ MODE (IF SO ABORT)
  for (int i = 0; i < NFD; i++)
  {
    if (running->fd[i].refCount != 0 && running->fd[i].mptr->ino == ino  && ( running->fd[i].mode > 0 || modeNum != 0) )
    {
      printf("File already opened.  Incompatible mode type requested.  Aborting open.\n");
      iput(mip);
      return -1;
    }
  }
  int i = 0;
  for (i; i < NFD; i++)
  {
    if (0 == running->fd[i].refCount)    // looking for open space in OFT
      break;
  }
  if (NFD == i)
  {
    printf("Limit for file descriptor table reached.  Aborting open.\n");
    return -1;
  }

  // FILLING IN VALUES FOR NEW FD
  OFT *new_fd = &(running->fd[i]);       // i is now set...so we can create a shortcut pointer to affected fd
  new_fd->mode = modeNum;
  new_fd->refCount = 1;
  new_fd->mptr = iget(mip->dev, mip->ino);
  iput(mip);
  switch (modeNum){
    case 0:
      new_fd->offset = 0;
      new_fd->mptr->inode.i_atime = time(0L);
      break;
    case 1:
      _truncate(new_fd->mptr);
      new_fd->offset = 0;
      new_fd->mptr->inode.i_atime = time(0L);
      new_fd->mptr->inode.i_mtime = time(0L);
      break;
    case 2:
      new_fd->offset = 0;
      new_fd->mptr->inode.i_atime = time(0L);
      new_fd->mptr->inode.i_mtime = time(0L);
      break;
    case 3:
      new_fd->offset = new_fd->mptr->inode.i_size;
      new_fd->mptr->inode.i_atime = time(0L);
      new_fd->mptr->inode.i_mtime = time(0L);
      break;
    default:
      printf("Mode selected as not 0-3.  Abort open");
      new_fd->refCount = 0;
      iput(new_fd->mptr);
      return -1;
      break;
  }

  new_fd->mptr->dirty = 1;
  return i;
}

int determineMode(char mode[BLKSIZE])
{
  if (!strcmp (mode, "r") || !strcmp (mode, "R"))
    return 0;
  if (!strcmp (mode, "w") || !strcmp (mode, "W"))
    return 1;
  if (!strcmp (mode, "rw") || !strcmp (mode, "RW"))
    return 2;
  if (!strcmp (mode, "a") || !strcmp (mode, "A"))
    return 3;
  return -1;
}

void pfd(int dev, PROC* running)
{
  printf("========================= PFD ==========================\n");
  printf("FD\tMode\tRefCount\tino\toffset\n");
  for (int i = 0; i < NFD; i++)
    if (running->fd[i].refCount != 0)
      printf("%d\t%d\t%d\t\t%d\t%d\n", i, running->fd[i].mode, running->fd[i].refCount, running->fd[i].mptr->ino, running->fd[i].offset);
  printf("--------------------------------------------------------\n");
  return;
}

void close_file(int dev, PROC *running, int fd_num)
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
  //RESET VARIABLES AND DECREMENT REFCOUNT
  oftp->mode = 0;
  oftp->refCount--;  //should be 0
  oftp->offset = 0;
  //IPUT mptr
  iput(oftp->mptr);
  oftp->mptr = NULL;
  return;
}
