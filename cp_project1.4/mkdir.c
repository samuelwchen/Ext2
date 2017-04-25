#include "project.h"

void _mkdir(int dev, PROC* running, char pathname[DEPTH][NAMELEN])
{
  MINODE *mip = NULL;
  //POINT MIP TO "STARTING" DIR
  if( !strcmp(pathname[0], "/") )
  {
    //ABSOLUTE
    mip = iget (dev, 2);
  }
  else
  {
    //RELATIVE
    mip = iget(running->cwd->dev, running->cwd->ino);
//    mip = running->cwd;
  }
  //GET NEW DIRECTORY NAME
  int i = 0;
  char new_dir_name[NAMELEN];
  for (i; ( strcmp(pathname[i], "\0") ) != 0; i++)
  {
    printf("_mkdir(): pathname[%d]: %s \n", i, pathname[i]);
  }
  printf("\n");

  int pino = 0;
  MINODE *pip = NULL;
  if(i == 0)
  {
    strcpy(new_dir_name, pathname[i]);
    strcpy(pathname[i], "\0");
  }
  else
  {
    strcpy(new_dir_name, pathname[i-1]);
    strcpy(pathname[i - 1], "\0");
  }

  printf("new_dir_name: %s\n", new_dir_name);
  if ( strcmp(pathname[0] ,"\0" ) == 0 )
  {
    //no pathname pip is mip
    pip = iget(mip->dev, mip->ino);
  }
  else
  {
    //GET MINODE OF PARENT DIRECTORY
    pino = getino(mip->dev, running, pathname);
    pip = iget(mip->dev, pino);
  }


  //CHECK PIP IS A DIR
  if ( !S_ISDIR(pip->inode.i_mode) )
  {
    //NOT A DIR - ABORT
    printf("Path does not exist. Aborting mkdir\n");

    iput(mip);
    iput(pip);
    return;
  }

  //CHECK IF FILE ALREADY EXISTS
  if( search(pip->dev, pip, new_dir_name) != 0)
  {
    //FILE EXISTS ALREADY
    printf("\"%s\" already exists. Aborting mkdir.\n", new_dir_name);
    iput(mip);
    iput(pip);
    return;
  }

  //CALL MY_MKDIR
  my_mkdir(running, pip, new_dir_name);
  //INCREMENT LINKS COUNT
  pip->inode.i_links_count++;
  //WRITE CHANGES BACK TO MEMORY
  pip->dirty = 1;
  mip->dirty = 1;
  iput(mip);
  iput(pip);
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
  dp->rec_len = BLKSIZE - 12;           //remaining space in block
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

  char indirect1_blk_buf[BLKSIZE];
  char indirect2_blk_buf[BLKSIZE];
  char indirect3_blk_buf[BLKSIZE];
  int *indirect1_blk_ptr = NULL;
  int *indirect2_blk_ptr = NULL;
  int *indirect3_blk_ptr = NULL;

  int name_entered = 0;
  for (int i = 0; i < 13; i++)//only direct blocks at the moment
  {
    //GET DATABLOCK NUMBER - this will change when we add indirect blocks
    if(i < 12)
    {
      name_entered = enter_name_helper(pip, &(pip->inode.i_block[i]), new_ideal_len, new_name_len, new_name, new_ino );
      if (name_entered == 1)
        return 1;
    }
    else if (i >= 12 && i < 13)
    {
      printf("*** Indirect blocks haven't been tested! Beware! ***\n");
      // WALKS THROUGH INODE_TABLE [12 TO 14] (INDIRECT BLOCKS)
      debugMode("At i_block[%d] = %d\n",index, pip->inode.i_block[i]);

      //GET INDIRECT BLOCK
      get_block(pip->dev, pip->inode.i_block[i], indirect1_blk_buf);
      indirect1_blk_ptr = (int *)indirect1_blk_buf;

      //WALK THROUGH INDIRECT BLOCK
      while (indirect1_blk_ptr < (int*)(indirect1_blk_buf + BLKSIZE) )
      {
        name_entered = enter_name_helper(pip, indirect1_blk_ptr, new_ideal_len, new_name_len, new_name, new_ino);
        if (name_entered == 1)
          return 1;
        indirect1_blk_ptr++;
      }
    }
  }
  return 0;
}
/********
helper function for name_enter
enters an entry in a DIR if space is available
else returns 0
i_block_ptr points at the i_block containing the DIR bno
*********/
int enter_name_helper(MINODE *pip, int *i_block_ptr, int new_ideal_len, int new_name_len, char *new_name, int new_ino)
{
  DIR *dp = NULL;
  char dp_buf[BLKSIZE];
  //CHECK IF LAST OF ALLOCATED BLOCKS
  if (*i_block_ptr == 0)  //DATABLOCK HAS NO ENTRIES YET
  {
    debugMode("...Allocating new data block");
    //ALLOCATE NEW DATABLOCK
    *i_block_ptr = balloc(pip->dev);

    //GET DATABLOCK
    get_block(pip->dev, *i_block_ptr, dp_buf);
    dp = (DIR *)dp_buf;

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
    put_block(pip->dev, *i_block_ptr, dp_buf);
    return 1;
  }
  else  //DATABLOCK HAS ENTRIES ALREADY
  {
    //GET DATABLOCK
    get_block(pip->dev, *i_block_ptr, dp_buf);
    dp = (DIR *)dp_buf;

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
      put_block(pip->dev, *i_block_ptr, dp_buf);

      return 1;
    }
    debugMode("Checking next i_block..\n");

  }
  return 0;
}
