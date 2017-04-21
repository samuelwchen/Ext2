#include "project.h"

void getInput(char cmd[64], char pathname[DEPTH][NAMELEN], char sourcePath[BLKSIZE], int dev, PROC *running)
{
  //char cmd[64], pathname[DEPTH][NAMELEN];
  char input[BLKSIZE] = {'\0'};
  char line[128] = {'\0'};
  char *token = NULL;
  char delim[2] = " ";

  sanitizePathname(pathname);

  printf("-------------------------------------------\n");

  //GET INPUT
  //pwd(dev, running->cwd);
  printf("[ls, pwd, cd, mkdir, test, quit]\n");
  printf("Enter command [pathname]: ");
  fgets(line, 128, stdin );


  sscanf(line, "%s %s %s", cmd, input, sourcePath);

  if (strcmp(cmd, "link") == 0)
  {
    char buf[BLKSIZE] = {'\0'};
    strcpy(buf, sourcePath);
    strcpy(input, sourcePath);
    strcpy(buf, input);
  }

  if (input[0] == '\0')
  {
    printf("input null\n");
    return;
  }
  parse(input, pathname);

  printf("cmd = \"%s\" pathname = \"", cmd);
  for (int i = 0; i < DEPTH && pathname[i][0] != '\0'; i++)
  {
    printf("%s/", pathname[i]);
  }
  printf("\"\n");
}

int main (int argc, char *argv[])
{
  printf("main()\n");
  char *disk = "mydisk";
  char line[128], cmd[64], pathname[DEPTH][NAMELEN];
  char input[BLKSIZE];
  char old_pathname[BLKSIZE];
  char buf[BLKSIZE];              // define buf1[ ], buf2[ ], etc. as you need
  char gpbuf[BLKSIZE];
  char ipbuf[BLKSIZE];
  char dpbuf[BLKSIZE];
  int fd = 0;

  MINODE *root = NULL;           // root pointer: the /
  PROC   proc[NPROC], *running;  // PROC; using only proc[0]

  SUPER *sp;
  GD    *gp;
  INODE *ip;
  DIR   *dp;

  //OPEN DISK
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  init(proc, root);

  checkMagicNumber(fd);
  printSuperBlock(fd);

  mount_root(fd, &root);

  //Need to SET FIRST PROCCESS and point running at it
  proc[0].cwd = root;
  //what do we do about the pid and next and etc?
  running = &proc[0];

  MINODE *mip = iget(fd, 2);
  printInode(&(mip->inode));
  iput(mip);

  while (1)
  {
    sanitizePathname(pathname);
    //pwd(fd, running->cwd);
    getInput(cmd, pathname, old_pathname, fd, running);
    printf("main() before call, running->cwd->refCount = %d, ino = %d\n", running->cwd->refCount, running->cwd->ino);
    if (!strcmp(cmd, "test"))
    {
      // int testNum = 2;
      // printf("enter inode number you wish to look for:: ");
      // scanf("%d", &testNum);
      // //strcpy(name, getNameFromIno(fd, testNum));  //A test
      // char name[NAMELEN] = {'\0'};
      // getNameFromIno(fd, testNum, name);  //A test
      // printf("Name found = %s\n", name);
      printf("IN TESTING AREA\n");
      //int allocated_bno = balloc(fd);
      //printf("Allocated block number %d\n", allocated_bno);
      //int allocated_ino = ialloc(fd);
      //printf("Allocated inode number %d\n", allocated_ino);

      //idealloc(fd, allocated_ino);
      //bdealloc(fd, allocated_bno);
      printf("---------------------\n");
      printSuperBlock(fd);
      my_mkdir(running, root, "testdir!");

      printSuperBlock(fd);
    }
    else if (!strcmp(cmd, "pwd"))
      pwd(fd, running->cwd);
    else if (!strcmp(cmd, "ls"))
      ls(fd, running, pathname);
    else if (!strcmp(cmd, "cd"))
      cd(fd, running, pathname);
    else if(!strcmp(cmd, "mkdir"))
      _mkdir(fd, running, pathname);
    else if (!strcmp(cmd, "rmdir"))
      rmDir (fd, running, pathname);
    else if (!strcmp(cmd, "symlink"))
      _symlink (fd, running, pathname, old_pathname);
    else if (!strcmp(cmd, "unlink"))
       _unlink (fd, running, pathname);
     else if (!strcmp(cmd, "create"))
        create (fd, running, pathname);
    else if (!strcmp(cmd, "quit"))
      quit();


    else if (!strcmp(cmd, "super"))
      printSuperBlock(fd);

    else if (!strcmp(cmd, "fill"))
      fillItUp(fd, running);
    else if (!strcmp(cmd, "fill2"))
      fillItUp2(fd, running);
    else if (!strcmp(cmd, "addSingle"))
      addSingleEntryBlock(fd, running);

    else
      printf("command %s was not recognized.\n\n", cmd);

  }

  quit();

}


void quit(void)
{
  debugMode("quit()\n");
  for (int i = 0; i < NMINODE; i++)
  {
    if (minode_table[i].refCount != 0)
      iput(&minode_table[i]);
  }

  exit(0);  // terminate program
}
