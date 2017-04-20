#include "project.h"

// void link(int dev, PROC* running, char pathname[DEPTH][NAMELEN], char sourcePath[BLKSIZE])
// {
//   char sourcePathName[DEPTH][NAMELEN];
//   parse(sourcePath, sourcePathName);
//
//   MINODE *mip = pathnameToMip(dev, running, sourcePathName);
//
//   search()
// //int search (dev, mip, char *name)
//
// }

void _symlink(int dev, PROC *running, char new_pathname_arr[DEPTH][NAMELEN], char old_pathname[BLKSIZE])
{
  //CHECKING THE OLD AND NEW PATH NAMES ARE CORRECT
  printf("***new_pathname: \"");
  for (int j = 0; j < DEPTH && strcmp(new_pathname_arr[j], "\0") != 0; j++ )
  {
    printf("%s/", new_pathname_arr[j]);
  }
  printf("\"\n");
  printf("****old_pathname: \"%s\"", old_pathname);

  //PARSE OLD_PATHANAME
  char old_pathname_arr[DEPTH][NAMELEN];
  parse(old_pathname, old_pathname_arr);  //parse will sanitize the pathname

  //CHECK IF OLD_PATHNAME_ARR EXISTS
  int old_ino = getino(dev, running, old_pathname_arr);
  if (old_ino == 0)
  {
    //OLD_PATHNAME_ARR DOES NOT EXISTS - ABORT
    printf("pathname: \"%s\" does not exit. Aborting symlink.\n", old_pathname);
    return;
  }

  //MOVE CHILD OUT OF NEW_PATHNAME_ARR
  int i = 0;
  for (i; i < DEPTH; i++) //just setting i to the last one
  {
    //CHECK IF LAST IN ARRAY
    if (i == (DEPTH -1) || strcmp(new_pathname_arr[i + 1], "\0") == 0 )
    {
      break;
    }
  }
  char new_filename[NAMELEN];
  strcpy(new_filename, new_pathname_arr[i]);
  strcpy(new_pathname_arr[i], "\0");

  //CHECK IF NEW_PATHNAME_ARR IS VALID PATH
  int pino = 0;
  if(strcmp(new_pathname_arr[0], "\0") == 0)
  {
    pino = running->cwd->ino;
  }
  else
  {
    pino = getino(dev, running, new_pathname_arr);
  }

  debugMode("symlink(): pino = %d\n", pino);
  if (pino == 0)
  {
    //OLD_PATHNAME_ARR DOES NOT EXISTS - ABORT
    printf("pathname: \"");
    for (int j = 0; j < DEPTH && strcmp(new_pathname_arr[i], "\0") != 0; j++ )
    {
      printf("%s/", new_pathname_arr[i]);
    }
    printf("\" does not exit. Aborting symlink.\n");
    return;
  }

  //CREATE SPECIAL LINK INODE AND INITIALIZE
  int new_ino = ialloc(dev);
  int new_bno = balloc(dev);
  if (new_ino == 0 || new_bno == 0)
  {
    printf("symlink(): ino = %d, bno = %d... Aborting symlink.\n", new_ino, new_bno);
    return;
  }
  MINODE *mip = iget(dev, new_ino);
  /******************************************/

  //INIT CONTENTS OF (symlink) MIP
  INODE *ip = &(mip->inode);
  ip->i_mode = 012777;      //Symlink type and permissions
  ip->i_uid = running->uid; //user's id
  ip->i_gid = 0;            //Group id MAY NEED TO CHANGE
  ip->i_size = BLKSIZE;
  ip->i_links_count = 1;    //Just 1 for itself
  ip->i_atime = time(0L);   //set to current time
  ip->i_ctime = ip->i_atime;//set to current time
  ip->i_mtime = ip->i_atime;//set to current time
  ip->i_blocks = 2;         //Linux: blocks count in 512-bytes chunks
  ip->i_block[0] = new_bno;     //Will need for storing the old_pathname

  //INSERT OLD_PATHNAME INTO THE DATABLOCK FOR I_BLOCK[0]
  //We have old_pathname[BLKSIZE], so we can skip copying it to a buf first
  put_block(mip->dev, ip->i_block[0], old_pathname);

  for (int i = 1; i < 15; i++)
  {
    ip->i_block[i] = 0;     //Unused blocks
  }
  mip->dirty = 1;           //mark dirty (so it will be written to mem)
  /******************************************/

  debugMode("symlink(): About to insert new link into pmip's directory\n\tpino = %d\n", pino);
  //INSERT NEW LINK ENTRY INTO PARENT DIRECTORY
  MINODE *pmip = iget(dev, pino);
  enter_name(pmip, new_ino, new_filename);

  //CLEAN UP, CLEAN UP, EVERYBODY DO YOUR PART
  iput(mip);
  iput(pmip);
}
