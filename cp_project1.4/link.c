#include "project.h"

void _link(int dev, PROC* running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE])
{
  char oldPathNameArray[DEPTH][NAMELEN];
  parse(oldPath, oldPathNameArray);

  // CHECK TO SEE IF TARGET FILE EXISTS AND IS VALID FOR LINKING
  MINODE *targetmip = pathnameToMip(dev, running, oldPathNameArray);

  if (0 == targetmip->ino)
  {
    printf("Target path does not exist.  Aborting link.\n");
    iput(targetmip);
    return;
  }
  if (!S_ISREG(targetmip->inode.i_mode) && !S_ISLNK(targetmip->inode.i_mode))
  {
    printf("Target is not a regular file or a link.  Aborting link.\n");
    iput(targetmip);
    return;
  }

  // CHECK TO SEE IF NEW LINK TO BE CREATED IS VALID
  char new_filename[NAMELEN] = {'\0'};
  getChildFileName(newPathNameArray, new_filename);

  MINODE *new_mip = pathnameToMip(dev, running, newPathNameArray);

  if(!S_ISDIR(new_mip->inode.i_mode))
  {
    printf("Not a valid pathname.  Cannot create link.  Aborting link.\n");
    iput(new_mip);
    iput(targetmip);
    return;
  }

  // LINKING!
  enter_name(new_mip, targetmip->ino, new_filename);
  new_mip->dirty = 1;
  iput(new_mip);
  iput(targetmip);

}

void getChildFileName(char new_pathname_arr[DEPTH][NAMELEN], char new_filename[NAMELEN])
{
  int i = 0;
  for (i; i < DEPTH; i++) //just setting i to the last one
  {
    //CHECK IF LAST IN ARRAY
    if (i == (DEPTH -1) || strcmp(new_pathname_arr[i + 1], "\0") == 0 )
    {
      break;
    }
  }

  strcpy(new_filename, new_pathname_arr[i]);
  // if (i == 0)
  //   strcpy(new_pathname_arr[i], "/");
  // else
  strcpy(new_pathname_arr[i], "\0");

}

void _symlink(int dev, PROC *running, char old_pathname[BLKSIZE], char new_pathname_arr[DEPTH][NAMELEN])
{
  char old_pathname_cp[BLKSIZE];
  strcpy(old_pathname_cp, old_pathname);//parse function changes old_pathname

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

  char new_filename[NAMELEN];
  getChildFileName(new_pathname_arr, new_filename);

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

  if (pino == 0)
  {
    //OLD_PATHNAME_ARR DOES NOT EXISTS - ABORT
    printf("pathname: \"");
    for (int j = 0, i = 0; j < DEPTH && strcmp(new_pathname_arr[i], "\0") != 0; j++, i++ )
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
  ip->i_mode = 0120777;      //Symlink type and permissions
  ip->i_uid = running->uid; //user's id
  ip->i_gid = 0;            //Group id MAY NEED TO CHANGE
  ip->i_size = BLKSIZE;
  ip->i_links_count = 1;    //Just 1 for itself
  ip->i_atime = time(0L);   //set to current time
  ip->i_ctime = ip->i_atime;//set to current time
  ip->i_mtime = ip->i_atime;//set to current time
  ip->i_blocks = 2;         //Linux: blocks count in 512-bytes chunks
  ip->i_block[0] = new_bno;     //Will need for storing the old_pathname

  //CHECK IF OLD_PATHNAME IS RELATIVE
  if (strcmp(old_pathname_arr[0], "\0") != 0 )
  {
    //OLD_PATHNAME IS RELATIVE MAKE ABOSLUTE
    char cwd_pathname[DEPTH][NAMELEN];
    pwdHelper(dev, running->cwd, cwd_pathname);

    strcpy(old_pathname_cp, "\0");
    for (int i = 1; i < DEPTH && cwd_pathname[i][0] != '\0'; i++)
    {
      strcat(old_pathname_cp, "/");
      strcat(old_pathname_cp, cwd_pathname[i]);
    }
    for (int i = 0; i < DEPTH && old_pathname_arr[i][0] != '\0'; i++)
    {
      strcat(old_pathname_cp, "/");
      strcat(old_pathname_cp, old_pathname_arr[i]);
    }
    printf("Linked to pathname: %s\n", old_pathname_cp);
  }

  //INSERT OLD_PATHNAME INTO THE DATABLOCK FOR I_BLOCK[0]
  //We have old_pathname[BLKSIZE], so we can skip copying it to a buf first
  put_block(mip->dev, ip->i_block[0], old_pathname_cp);

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
/*
  Gets the last filname in the pathname stored in the SYMLINK minode and puts
  it in filename.
  Used to print (symlink -> filename)
*/
void readLink (int dev, PROC* running, char sym_pathname_arr[DEPTH][NAMELEN])
{
  //GET MINODE OF THE SYMLINK
  int ino = getino( dev, running, sym_pathname_arr);
  MINODE *mip = iget(dev, ino);

  //CHECK IF MIP IS A SYMLINK
  if(S_ISLNK(mip->inode.i_mode))
  {
    //GET THE PATHANME FROM THE I_BLOCK[0]
    char pathname[BLKSIZE] = {'\0'};
    char pathname_arr[DEPTH][NAMELEN];
    get_block(mip->dev, mip->inode.i_block[0], pathname);

    //PARSE PATHNAME
    //parse(pathname, pathname_arr);

    //GET LAST FILENAME
    //char filename[NAMELEN] = {'\0'};
    char linkname[NAMELEN] = {'\0'};
    //getChildFileName(pathname_arr, filename);
    getChildFileName(sym_pathname_arr, linkname);

    printf("%s -> %s\n", linkname, pathname);
  }
  else
  {
    debugMode("readLink(): NOT A SYMLINK\n");
  }
  iput (mip);
}

int readSymLink(int dev, PROC* running, MINODE *mip)
{
  //CHECK IF MIP IS A SYMLINK
  if(!S_ISLNK(mip->inode.i_mode))
  {
    printf("Not a symLink. Abort readSymLink.\n");
    return 0;
  }
  //GET THE PATHANME FROM THE I_BLOCK[0]
  char pathname[BLKSIZE] = {'\0'};
  char pathname_arr[DEPTH][NAMELEN];
  get_block(mip->dev, mip->inode.i_block[0], pathname);

  //PARSE PATHNAME

  parse(pathname, pathname_arr);

  //GETINO AND RETURN THE INODE NUMBER THE LINK GOES TO
  int ino = getino(mip->dev, running, pathname_arr);
  return ino;

}

void _unlink(int dev, PROC *running, char pathname[DEPTH][NAMELEN])
{

  MINODE* mip = pathnameToMip(dev, running, pathname);
  MINODE* pmip = NULL;
  char filename[NAMELEN];
  getChildFileName(pathname, filename);

  if (mip == NULL)
    return;

  // WE HAVE PATHNAME 

  // CHECKING IF DIRECTORY IS VALID FOR DELETION
  if (mip == 0)
  {
    printf("No such directory.\n");
    return;
  }

  if (! (S_ISLNK(mip->inode.i_mode) || S_ISREG(mip->inode.i_mode)))
  {
    printf("%s is not a file or symlink.\n", filename);
    return;
  }

  debugMode("refCount = %d", mip->refCount);
  if (mip->refCount != 1)
  {
    printf("Directory being used by %d other programs.  Abort unlink.\n", mip->refCount - 1);
    return;
  }


  // DELETION NOW POSSIBLE.  PREPARING TO DELETE

  if (strcmp(pathname[0], "\0") == 0)
    pmip = iget(dev, running->cwd->ino);
  else
    pmip = iget(dev, getino(dev, running, pathname));


  rmDirEntry(dev, pmip, mip);

}
