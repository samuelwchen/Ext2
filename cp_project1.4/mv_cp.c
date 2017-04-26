#include "project.h"

void mv(int dev, PROC *running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE])
{

  char oldPathNameArray[DEPTH][NAMELEN];
  parse(oldPath, oldPathNameArray);

  // CHECK TO SEE IF TARGET FILE EXISTS AND IS VALID TO BE MOVED
  MINODE *old_mip = pathnameToMip(dev, running, oldPathNameArray);

  if (NULL == old_mip)
  {
    printf("Target path does not exist.  Aborting mv.\n");
    return;
  }
  if (!S_ISREG(old_mip->inode.i_mode) && !S_ISLNK(old_mip->inode.i_mode))
  {
    printf("Target is not a regular file or a link.  Aborting mv.\n");
    iput(old_mip);
    return;
  }

  // CHECK TO SEE IF THE FILE IS OPENED
  for (int i = 0; i < NFD; i++)
  {
    if (running->fd[i].refCount != 0 && running->fd[i].mptr->ino == old_mip->ino  && running->fd[i].mode > 0 )
    {
      printf("File already opened.  Aborting mv.\n");
      iput(old_mip);
      return ;
    }
  }

  // CHECK TO SEE IF NEW FILENAME IS ALREADY TAKEN
  char new_filename[NAMELEN] = {'\0'};
  getChildFileName(newPathNameArray, new_filename);

  MINODE *new_mip = pathnameToMip(dev, running, newPathNameArray);

  if(!S_ISDIR(new_mip->inode.i_mode))
  {
    printf("Not a valid pathname.  Cannot create link.  Aborting mv.\n");
    iput(new_mip);
    iput(old_mip);
    return;
  }

  // CHECK TO SEE IF TARGET FILE NAME ALREADY EXISTS


  int ino = search (dev, new_mip, new_filename);
  //MINODE *old_mip = iget(dev, ino);
  if (ino != 0)
  {
    printf("Target filename already exists.  Aborting mv.\n");
    iput(new_mip);
    iput(old_mip);
    return;
  }


  // MOVING
  enter_name(new_mip, old_mip->ino, new_filename);
  new_mip->dirty = 1;
  iput(new_mip);
  iput(old_mip);
  _unlink(dev, running, oldPathNameArray);


}
