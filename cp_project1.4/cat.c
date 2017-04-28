#include "project.h"

void cat (int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{
  // CHECK IF FILE IS OPENED
  int fd_num = open_file(dev, running, pathname, "r");
  if (fd_num < 0)
  {
    printf("Cannot open file.  Aborting cat.\n");
    return;
  }

  // READ AND PRINT FILE
  printRead(running, fd_num, running->fd[fd_num].mptr->inode.i_size);

  // CLOSE FILE
  close_file(dev, running, fd_num);
  return;
}
