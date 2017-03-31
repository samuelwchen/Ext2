#include "project.h"

void parse(char input[STRLEN], char pathname[DEPTH][NAMELEN])
{
  printf("parse()\n");
  char delim[2] = "/";
  char *token;
  int i = 0;

  for (int j = 0; j < DEPTH; j++)
  {
    strcpy(pathname[j], "\0");
  }

  if (input[0] == '/')
  {
    strcpy(pathname[i], "/");
    i = 1;
  }

  //get first token
  token = strtok(input, delim);

  for(i;token != NULL; i++)
  {
    strcpy(pathname[i], token);
    token = strtok(NULL, delim);
  }

}

/* Returns the inode number */
int search (int dev, MINODE *mip, char *name)
{
  printf("search()\n");
  char buf[BLKSIZE];
  char *cp;
  DIR *dp;
  char dirName[256];
  u32 iblock;
  int i = 0;

  printf("Name of file to find == %s\n", name);
  printInode(&(mip->inode));

  for (int index = 0; index < 12; index++)    // defend against table[12, 13, 14]
  {
    iblock = mip->inode.i_block[index];
    printf("--------------------------------------------\n");
    printf("At i_block[%d] = %d\n", index, iblock);
    get_block(dev, iblock, buf);

    dp = (DIR*)buf;

    if (dp->rec_len == 0)
    {
      printf("reclen = 0\n");
      break;
    }

    while(dp < (DIR*)(buf + BLKSIZE))
    {
      for (i = 0; i < dp->name_len; i++)
      {
        dirName[i] = dp->name[i];
      }
      dirName[i] = '\0';

      printf("Directory Name = %s \n",dirName);
      if (strcmp(dirName, name) == 0)   // target found
      {
        printf("File FOUND.  Inode # = %d\n", dp->inode);
        printf("--------------------------------------------\n");
        return dp->inode;
      }

      dp = (DIR*)((char*)dp + dp->rec_len);
    }

  }
  printf("File NOT found.\n");
  return 0;
}


void ls(int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{
  int ino = 0;
  MINODE *mip = NULL;
  DIR *dp = NULL;
  char dirName[NAMELEN];

  if ( !strcmp(pathname[0], "/") )
     mip = iget(dev, 2);
  else
     mip = iget(running->cwd->dev, running->cwd->ino);

  // convert pathname to (dev, ino);
  // get a MINODE *mip pointing at a minode[ ] with (dev, ino);
  ino = getino(mip->dev, running, pathname);
  if (ino == 0)
  {
    printf("Dir path does not exists.\n");
    return;
  }
  mip = iget(mip->dev, ino);

  char buf[BLKSIZE];

  int i = 0, j = 0;

  printf("==================== ls ====================\n");
  while(mip->inode.i_block[i] != 0 && i < 15)
  {
    get_block(dev, mip->inode.i_block[i], buf);
    dp = (DIR *)buf;

    while (dp < (DIR*)(buf + BLKSIZE))
    {
      for (j = 0; j < dp->name_len; j++)
      {
        dirName[j] = dp->name[j];
      }
      dirName[j] = '\0';
      printf("%s\n", dirName);

      dp = (DIR*)((char*)dp + dp->rec_len);
    }
    i++;
  }
  printf("===============================================\n");
  // if mip->INODE is a file: ls_file(pathname);
  // if mip->INODE is a DIR{
  //    step through DIR entries:
  //    for each name, ls_file(pathname/name);
  // }
  //if()
}

void cd(int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{
  int ino = 0;
  MINODE *mip = NULL;
  DIR *dp = NULL;
  char buf[BLKSIZE];
  char dirName[NAMELEN];

  if ( !strcmp(pathname[0], "/") )
     mip = iget(dev, 2);
  else
     mip = iget(running->cwd->dev, running->cwd->ino);

  // convert pathname to (dev, ino);
  // get a MINODE *mip pointing at a minode[ ] with (dev, ino);
  ino = getino(mip->dev, running, pathname);
  if (ino == 0)
  {
    printf("Dir path does not exists.\n");
    return;
  }
  mip = iget(mip->dev, ino);



  if(!S_ISDIR(mip->inode.i_mode))
  {
    printf("Not a Directory\n");
    return;
  }
  iput(running->cwd);
  running->cwd = mip;


}
