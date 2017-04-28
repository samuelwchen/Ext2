#include "project.h"

void mv(int dev, PROC *running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE])
{

  char oldPathNameArray[DEPTH][NAMELEN];
  parse(oldPath, oldPathNameArray);
  MINODE *old_mip = pathnameToMip(dev, running, oldPathNameArray);

  // CHECK TO SEE IF TARGET FILE EXISTS AND IS VALID TO BE MOVED
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
  return;
}

void copy(int dev, PROC *running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE])
{
  char oldPathNameArray[DEPTH][NAMELEN];
  parse(oldPath, oldPathNameArray);
  MINODE *old_mip = pathnameToMip(dev, running, oldPathNameArray);

  // CHECK TO SEE IF TARGET FILE EXISTS AND IS VALID TO BE MOVED
  if (NULL == old_mip)
  {
    printf("Target path does not exist.  Aborting mv.\n");
    return;
  }
  if (!S_ISREG(old_mip->inode.i_mode) && !S_ISLNK(old_mip->inode.i_mode))
  {
    printf("Target is not a regular file or a link.  Aborting cp.\n");
    iput(old_mip);
    return;
  }

  // CHECK TO SEE IF THE FILE IS OPENED
  for (int i = 0; i < NFD; i++)
  {
    if (running->fd[i].refCount != 0 && running->fd[i].mptr->ino == old_mip->ino  && running->fd[i].mode > 0 )
    {
      printf("File already opened.  Aborting cp.\n");
      iput(old_mip);
      return ;
    }
  }

  // CHECK TO SEE IF NEW FILENAME IS ALREADY TAKEN
  char new_filename[NAMELEN] = {'\0'};
  getChildFileName(newPathNameArray, new_filename);

  // CHECK TO SEE IF THE NEW FILE NAME IS ""
  if(!strcmp(new_filename, "") || new_filename[0] == '\0')
  {
    printf("Cannot copy to a file name \"\"\n");
    return;
  }

  MINODE *new_mip = pathnameToMip(dev, running, newPathNameArray);

  if(!S_ISDIR(new_mip->inode.i_mode))
  {
    printf("Not a valid pathname.  Cannot copy a directory.  Aborting cp.\n");
    iput(new_mip);
    iput(old_mip);
    return;
  }

  // CHECK TO SEE IF TARGET FILE NAME ALREADY EXISTS
  int ino = search (dev, new_mip, new_filename);
  if (ino != 0)
  {
    printf("Target filename already exists.  Aborting cp.\n");
    iput(new_mip);
    iput(old_mip);
    return;
  }

  //  CREATING NEW FILE TO COPY INTO
  char cp_newPathNameArray[DEPTH][NAMELEN];
  for(int i = 0; i < DEPTH; i++)    // reattaching filename to the path array.  setting up for create funciton
  {
    strcpy(cp_newPathNameArray[i], newPathNameArray[i]);
    if(!strcmp(newPathNameArray[i], "\0"))
    {
      strcpy(cp_newPathNameArray[i], new_filename);
      strcpy(newPathNameArray[i], new_filename);
      break;
    }
  }
  create (dev, running, cp_newPathNameArray);     // cp_newPathNameArray has been parsed and destroyed.  DO NOT USE AGAIN
  ino = getino(dev, running, newPathNameArray);

  new_mip->dirty = 1;
  iput(new_mip);
  new_mip = iget(dev, ino);

  // COPYING
  int n_bytesToCopy = old_mip->inode.i_size;   // determining how much to copy
  int old_fd = open_file(dev, running, oldPathNameArray, "r");
  int new_fd = open_file(dev, running, newPathNameArray, "w");

  char buf[BLKSIZE];
  int bytesRead = 0;
  while(n_bytesToCopy > 0)
  {
    // READING FROM SOURCE
    int bytesReadInBlk = 0;
    if (n_bytesToCopy < BLKSIZE)
      bytesReadInBlk = _read(&(running->fd[old_fd]), buf, n_bytesToCopy);
    else
      bytesReadInBlk = _read(&(running->fd[old_fd]), buf, BLKSIZE);
    bytesRead += bytesReadInBlk;
    running->fd[old_fd].offset = bytesRead;

    // WRITING TO TARGET DESTINATION
    if (n_bytesToCopy < BLKSIZE)
      _write(&(running->fd[new_fd]), buf, n_bytesToCopy);
    else
      _write(&(running->fd[new_fd]), buf, BLKSIZE);
    running->fd[new_fd].offset = bytesRead;
    new_mip->inode.i_size = bytesRead;

    // CLEANING UP
    n_bytesToCopy -= bytesReadInBlk;
  }
  new_mip->dirty = 1;
  iput(new_mip);
  iput(old_mip);

  //CLOSE THE FILES
  close_file(dev, running, new_fd);
  close_file(dev, running, old_fd);

  return;
}
