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
  debugMode("parse()\n");
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
  debugMode("search()\n");
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
  debugMode("ls()\n");
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


  printf("==================== ls ====================\n");
  printf("Permissions\tFile Size\tName\n");
  int i = 0, j = 0;
  char buf[BLKSIZE];
  MINODE* childmip = NULL;
  while(mip->inode.i_block[i] != 0 && i < 15)
  {
    get_block(dev, mip->inode.i_block[i], buf);
    dp = (DIR *)buf;

    // PRINTING READ, WRITE, EXECUTABLE PERMISSIONS, FILE/DIR NAME
    while (dp < (DIR*)(buf + BLKSIZE))
    {
      childmip = iget(dev, dp->inode);
      printf( (S_ISDIR(childmip->inode.i_mode)) ? "d" : "-");
      printf( (childmip->inode.i_mode & S_IRUSR) ? "r" : "-");
      printf( (childmip->inode.i_mode & S_IWUSR) ? "w" : "-");
      printf( (childmip->inode.i_mode & S_IXUSR) ? "x" : "-");
      printf( (childmip->inode.i_mode & S_IRGRP) ? "r" : "-");
      printf( (childmip->inode.i_mode & S_IWGRP) ? "w" : "-");
      printf( (childmip->inode.i_mode & S_IXGRP) ? "x" : "-");
      printf( (childmip->inode.i_mode & S_IROTH) ? "r" : "-");
      printf( (childmip->inode.i_mode & S_IWOTH) ? "w" : "-");
      printf( (childmip->inode.i_mode & S_IXOTH) ? "x\t" : "-\t");
      printf("%d\t\t", childmip->inode.i_size);

      for (j = 0; j < dp->name_len; j++)
      {
        dirName[j] = dp->name[j];
      }
      dirName[j] = '\0';
      printf("%s\n", dirName);

      dp = (DIR*)((char*)dp + dp->rec_len);

      iput(childmip);
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
  debugMode("cd()\n");
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
  debugMode("getNameFromInoHelper()\n");
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
  debugMode("getNameFromIno()\n");
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

void _mkdir(int dev, PROC* running, char pathname[DEPTH][NAMELEN])
{

}

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

  //"ALLOCATE" MIP
  MINODE *mip = iget(pip->dev, ino);

  //INIT CONTENTS OF MIP
  INODE *ip = &(mip->inode);
  ip->i_mode = 040755;      //DIR type and permissions
  ip->i_uid = running->uid; //user's id
  ip->i_gid = 0;            //Group id MAY NEED TO CHANGE
  ip->i_size = BLKSIZE;
  ip->i_links_count = 2;    //links for . and ..
  ip->i_atime = time(0L);   //set to current time
  ip->i_ctime = ip->i_atime;//set to current time
  ip->i_mtime = ip->i_atime;//set to current time
  ip->i_blocks = 2;         //Linux: blocks count in 512-bytes chunks
  ip->i_block[0] = bno;     //Will need for dir entries . and ..
  for (int i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;     //Unused blocks
  }
  mip->dirty = 1;           //mark dirty (so it will be written to mem)

  //WRITE MIP BACK TO MEMORY (DEALLOCATE IT)
  iput(mip);

  //INIT THE NEW DATABLOCK (I_BLOCK[0])
  char dp_buf[BLKSIZE];
  DIR *dp = (DIR *)dp_buf;
  char *cp = NULL;

  //INIT THE FIRST ENTRY "."
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  cp = (char *)(dp->name);
  *cp = '.';

  //INIT SECOND ENTRY ".."
  dp = (DIR *)( (char *)dp + dp->rec_len);
  dp->inode = pip->ino;
  dp->rec_len = 1012;           //remaining space in block
  dp->name_len = 2;
  cp = (char *)(dp->name);
  *cp = '.';
  cp++;
  *cp = '.';

  //WRITE NEW DATABLOCK BACK TO MEMORY
  put_block(pip->dev, bno, dp_buf);

  //add name to parent's directry
  enter_name(pip, ino, new_name);
}
//entername enters a new directy entry in the parent's datablock
// returns
// 1 on sucess and 0 on failure
int enter_name(MINODE *pip, int new_ino, char *new_name)
{
  //CALCULATING IDEAL_LENGTH OF NEW RECORD
  int new_name_len = 0;
  for (int i = 0; i < 256 && new_name[i] != '\0'; i++)
  {
    new_name_len++;
  }
  int new_ideal_len = 8 + ( 4 * ( (new_name_len + 3)/4 ) );

  //LOOP THROUGH PARENT'S DIR ENTRIES TO FIND AVAILABLE SPACE
  int bno = 0;
  DIR *dp = NULL;
  char dp_buf[BLKSIZE];
  for (int i = 0; i < 12; i++)//only direct blocks at the moment
  {
    //GET DATABLOCK NUMBER - this will change when we add indirect blocks
    bno = pip->inode.i_block[i];

    //CHECK IF LAST OF ALLOCATED BLOCKS
    if (bno == 0)  //DATABLOCK HAS NO ENTRIES YET
    {
      debugMode("i_block[%d]... Allocating new data block", i);
      //ALLOCATE NEW DATABLOCK
      bno = balloc(pip->dev);
      pip->inode.i_block[i] = bno;

      //GET DATABLOCK
      get_block(pip->dev, pip->inode.i_block[i], dp_buf);
      dp = dp_buf;

      //ADD NEW ENTRY
      dp->inode = new_ino;
      dp->rec_len = BLKSIZE;
      dp->name_len = new_name_len;
      char *cp = (char *)(dp->name);
      for (int j = 0; j < NAMELEN && new_name[j] != '\0'; j++)
      {
        *cp = new_name[j];
        cp++;
      }
      //WRITE BLOCK BACK TO DISK
      put_block(pip->dev, bno, dp_buf);
      return 1;
    }
    else  //DATABLOCK HAS ENTRIES ALREADY
    {
      //GET DATABLOCK
      get_block(pip->dev, pip->inode.i_block[i], dp_buf);
      dp = dp_buf;

      //WALK TO LAST DIR ENTRY
      while ((char *)dp + dp->rec_len < dp_buf + BLKSIZE)
      {
        dp = (DIR *) ( (char *)dp + dp->rec_len );
      }

      debugMode("i_block[%d]: (Last entry)\n");
      //CALCULATE IDEAL LENGTH OF LAST ENTRY
      int last_ideal_len = 8 + ( 4 * ( (dp->name_len + 3)/4 ) );
      int remaining_space = dp->rec_len - last_ideal_len;

      //CHEACK AVAILABLE SPACE
      if (new_ideal_len <= remaining_space)
      {
        //CHANGE CURRENT REC_LEN TO ITS IDEAL LENGTH
        dp->rec_len = last_ideal_len;

        //ENTER NEW ENTRY
        debugMode("dp -> %d, last_ideal_len = %d\n", dp, last_ideal_len);

        //MOVE DP TO AVAILABLE SPACE
        dp = (DIR *) ( (char *)dp + last_ideal_len );
        debugMode("dp+last_ideal_len -> %d\n", dp);

        dp->inode = new_ino;
        dp->rec_len = remaining_space;
        dp->name_len = new_name_len;
        char *cp = (char *)(dp->name);
        for (int j = 0; j < NAMELEN && new_name[j] != '\0'; j++)
        {
          *cp = new_name[j];
          cp++;
        }
        //WRITE BLOCK BACK TO DISK
        put_block(pip->dev, pip->inode.i_block[i], dp_buf);
        return 1;
      }
      debugMode("Checking next i_block..\n");
    }
  }
  return 0;
}
