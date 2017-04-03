#include "project.h"

/**************************************************
Precondition :: None.
Info :: Resets all pathname array to ''\0'
**************************************************/
void sanitizePathname(char pathname[DEPTH][NAMELEN])
{
  for (int i = 0; i < DEPTH; i++)
    //pathname[i][0] = '\0';
    strcpy(pathname[i], "\0");
}

/**************************************************
Precondition :: None.
Info :: Parse a string, delimitedon '/'.  If front of string
contains '/', set parse[0] to root.
**************************************************/
void parse(char input[STRLEN], char pathname[DEPTH][NAMELEN])
{
  printf("parse()\n");
  char delim[2] = "/";
  char *token;
  int i = 0;

  sanitizePathname(pathname);

  if (input[0] == '/')
  {
    strcpy(pathname[i], "/");
    i = 1;              // root occupies pathname[0]
  }

  // PARSE INPUT INTO PATHNAME ARRAY
  token = strtok(input, delim);
  for(i;token != NULL; i++)
  {
    strcpy(pathname[i], token);
    token = strtok(NULL, delim);
  }
}

/**************************************************
Precondition :: None.
Info :: Walks through a mip looking for a target file/dir
name.  Once found, returns the inode of target
**************************************************/
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

  // WALKS THROUGH INODE_TABLE [0 TO 11] (DIRECT BLOCKS)
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

    while(dp < (DIR*)(buf + BLKSIZE))   // while dp is still inside buffer
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

  // WALKS THROUGH INODE_TABLE [12 TO 14] (INDIRECT BLOCKS)
  for (int i = 12; i < 15; i++)
  {
    printf("At i_block[%d] = %d\n",index, mip->inode.i_block[i]);
    if(mip->inode.i_block[i] == 0)
      return 0;
    return searchHelper(dev, i-11, mip->inode.i_block[i], i);
  }

  // DID NOT FIND TARGET FILE
  printf("File NOT found.\n");
  return 0;
}

/**************************************************
Precondition :: all ints must of non-negative.
Info :: Gets called by search.  recursively searches through
indirect blocks until block number of interest is found.

**************************************************/
int searchHelper(int dev, int level_indirection, int block_num, int inode_table_index)
{
  if (level_indirection == 0)
  {
    printf("i_block[%d] \t block_num = %d\n", inode_table_index, block_num);
    return block_num;
  }

  //getting the indirect block
  char buf[BLKSIZE];
  int *pIndirect_blk = NULL;

  get_block(dev, block_num, buf);
  pIndirect_blk = (int*)buf;      //points to the array of block numbers

  // WALKING THROUGH ALL THE BLOCK NUMBERS IN A BLOCK.
  for (int i = 0; i < BLKSIZE/sizeof(int); i++)
  {
    if (*pIndirect_blk == 0)
      return 0;
    block_num = searchHelper(dev, level_indirection-1, *pIndirect_blk, inode_table_index);
    if (block_num != 0)
      return block_num;
    pIndirect_blk++;
  }
  return 0;
}

/**************************************************
Precondition :: None
Info :: displays what files/dir are in directory.
Can use absolute or relative path
**************************************************/
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
  if (!(pathname[0][0] == '\0'))
  {
    ino = getino(mip->dev, running, pathname);
    if (ino == 0)
    {
      printf("Dir path does not exists.\n");
      return;
    }
    mip = iget(mip->dev, ino);
  }

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
}
/**************************************************
Precondition :: none
Info :: changes directory.
**************************************************/
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


void pwd(int dev, MINODE* mip)
{
  printf("\n");
  pwdHelper(dev, mip);
  printf("\n");
}

void pwdHelper(int dev, MINODE* mip)
{
  if (mip->ino == 2)
  {
    //fflush(stdout);
    printf("Current Directory = /");
    return;
  }
  // GETTING PARENT mip
  int pino = search(dev, mip, "..");
  MINODE* pmip = iget(dev, pino);
  pwdHelper(dev, pmip);

  char fileName[NAMELEN] = {'\0'};
  getNameFromIno(dev, mip->ino, fileName);
  printf("%s/", fileName);
}

/*
Info :: returns a 1 if successfully found.  returns a 0 if unsuccessful
*/
int getNameFromInoHelper(int dev, int level_indirection, int block_num, int ino, char fileName[NAMELEN])
{
  printf("getNameFromInoHelper()\n");
  char buf[BLKSIZE] = {'\0'};
  int *pIndirect_blk = NULL;
  int success = 0;

  DIR *dp = NULL;
  if (level_indirection == 0)
  {
    printf("Indirect Block block_num = %d\n", block_num);

    //search block for ino
    get_block (dev, block_num, buf);
    dp = (DIR *)buf;

    //SEARCH DIR ENTRIES FOR INO
    while ( dp < (DIR *)(buf + BLKSIZE) )
    {
      if (dp->inode == ino)
      {
        //CREATE NULL TERMINATED STRING TO RETURN
        char *cp = dp->name;
        int i = 0;
        for (; i < dp->name_len; i++ )
          fileName[i] = *cp;
        fileName[i] = '\0';

        return 1;
      }
      dp = (DIR *)( (char *)dp + dp->rec_len );
    }
    return 0;
  }

  //LEVEL INDIRECTION != 1 (ie. block  contains ino's, not dir entries)
  get_block(dev, block_num, buf);
  pIndirect_blk = (int*)buf;      //points to the array of block numbers


  // WALKING THROUGH ALL THE BLOCK NUMBERS IN AN INDIRECT BLOCK.

  for (int i = 0; i < BLKSIZE/sizeof(int); i++)
  {
    if (*pIndirect_blk == 0)
      return 0;
    success = getNameFromInoHelper(dev, level_indirection-1, *pIndirect_blk, ino, fileName);
    if (success != 0)
      return success;
    pIndirect_blk++;
  }
  return 0;
}

void getNameFromIno(int dev, int ino, char fileName[NAMELEN])
{
  printf("getNameFromIno()\n");
  char buf[BLKSIZE];
  char *cp;
  DIR *dp;
  u32 iblock;
  int i = 0;

  MINODE *mip = iget(dev, ino);
  int pino = search(dev, mip, "..");
  mip = iget(dev, pino);

    // WALKS THROUGH INODE_TABLE [0 TO 11] (DIRECT BLOCKS)
  for (int index = 0; index < 12; index++)    // defend against table[12, 13, 14]
  {
    iblock = mip->inode.i_block[index];
    get_block(dev, iblock, buf);

    dp = (DIR*)buf;

    if (dp->rec_len == 0)
    {
      printf("reclen = 0\n");
      break;
    }

    while(dp < (DIR*)(buf + BLKSIZE))   // while dp is still inside buffer
    {
      for (i = 0; i < dp->name_len; i++)
      {
        fileName[i] = dp->name[i];
      }
      fileName[i] = '\0';

      printf("Directory Name = %s \n",fileName);
      if (dp->inode == ino)   // target found
      {
        printf("File FOUND.  Inode # = %d, name = %s\n", dp->inode, fileName);
        printf("--------------------------------------------\n");
        return;
      }

      dp = (DIR*)((char*)dp + dp->rec_len);
    }
  }

  // WALKS THROUGH INODE_TABLE [12 TO 14] (INDIRECT BLOCKS)
  for (int i = 12; i < 15; i++)
  {
    printf("At i_block[%d] = %d\n",index, mip->inode.i_block[i]);
    if(mip->inode.i_block[i] == 0)
      return;
    return getNameFromInoHelper(dev, i-11, mip->inode.i_block[i], ino, fileName);//will return null if not found
  }

  // DID NOT FIND TARGET FILE
  printf("File NOT found.\n");
  return;

}

// void mkdir(int dev, PROC* running, char pathname[DEPTH][NAMELEN])
// {
//
// }

//Precondition: pip is a dir
void my_mkdir(PROC *running, MINODE *pip, char *new_name)
{
  //GET INO AND BNO (AND CHECK IF SPACE IS AVAILABLE)
  int ino = ialloc(pip->dev);
  int bno = balloc(pip->dev);

  if (bno == 0 | ino == 0)
  {
    printf("my_mkdir(): bno = %d, ino = %d", bno, ino);
    return;
  }


}
