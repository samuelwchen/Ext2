#include "project.h"

/*****************************************************************************
Similar to mkdir
******************************************************************************/
void create(int dev, PROC* running, char pathname[DEPTH][NAMELEN])
{
  //GET NEW FILE NAME
  char filename[NAMELEN];
  getChildFileName(pathname, filename);
  MINODE *pmip = NULL;
  if (strcmp(pathname[0], "\0") == 0)
    pmip = iget(dev, running->cwd->ino);
  else
    pmip = iget(dev, getino(dev, running, pathname));

  //CHECK IF FILE ALREADY EXISTS
  if( search(pmip->dev, pmip, filename) != 0)
  {
    //FILE EXISTS ALREADY
    printf("\"%s\" already exists. Aborting mkdir.\n", filename);
    iput(pmip);
    return;
  }

  //CALL CREATE_HELPER (sets of INODE's initial values)
  createHelper(running, pmip, filename);

  //INCREMENT LINKS COUNT
  pmip->inode.i_links_count++;

  //WRITE CHANGES BACK TO MEMORY
  pmip->dirty = 1;
  iput(pmip);
  //mip->dirty = 1;
  //iput(mip);
}

/*****************************************************************************
Sets the initial values in INODE that was called by CREATE
******************************************************************************/
void createHelper(PROC *running, MINODE *pmip, char *filename)
{
  //GET INO AND BNO (AND CHECK IF SPACE IS AVAILABLE)
  int ino = ialloc(pmip->dev);
  int bno = balloc(pmip->dev);

  if (bno == 0 | ino == 0)
  {
    printf("createHelper() - Out of inodes or blocks: bno = %d, ino = %d", bno, ino);
    return;
  }

  //"ALLOCATE" MIP
  MINODE *mip = iget(pmip->dev, ino);

  //INIT CONTENTS OF MIP
  INODE *ip = &(mip->inode);
  ip->i_mode = 0100644;      //DIR type and permissions
  ip->i_uid = running->uid; //user's id
  ip->i_gid = 0;            //Group id MAY NEED TO CHANGE
  ip->i_size = 0;
  ip->i_links_count = 1;
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

  //add name to parent's directry
  enter_name(pmip, ino, filename);
}
