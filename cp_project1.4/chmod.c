#include "project.h"

void _chmod(int dev, PROC *running, char pathname[DEPTH][NAMELEN], char value[BLKSIZE])
{
  if (strlen(value) != 4)
  {
    printf("\n--------------------------------------------------\n");
    printf("Incorrect value for chmod. Must by 4 octal numbers.\n");
    return;
  }

  int permissions[4] = {0};

  for(int i = 0; i < 4; i++)
  {
    if (value[i] - '0' >= 0 && value[i] - '0' <= 7)
    {
      permissions[i] = value[i] - '0';
    }
    else
    {
      printf("--------------------------------------------------\n");
      printf("Incorrect value for chmod.  Must by 4 octal numbers.\n");
      return;
    }
  }

  int ino = getino(dev, running, pathname);
  MINODE *mip = iget(dev, ino);

  if (mip == 0)
  {
    printf("---------------------------------------------------\n");
    printf ("Path does not exist.  Aborting chmod. /n");
    return;
  }

  int tempMode = (int)(mip->inode.i_mode);
  tempMode = tempMode >> 12;

  for (int i = 0; i < 4; i++)
  {
    tempMode = tempMode << 3;
    tempMode = tempMode | permissions[i];
  }
  mip->inode.i_mode = tempMode;
  return;


}
