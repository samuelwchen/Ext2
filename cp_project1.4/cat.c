#include "project.h"

void cat (int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{
  int fd_num = open_file(dev, running, pathname, "r");
  if (fd_num < 0)
  {
    printf("Cannot open file.  Aborting cat.\n");
    return;
  }
  printRead(running, fd_num, running->fd[fd_num].mptr->inode.i_size);
  close_file(dev, running, fd_num);
  return;
}
