#include "project.h"

void getInput(char cmd[64], char pathname[DEPTH][NAMELEN], int dev, PROC *running)
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
  printf("Enter command [pathname]: ");
  fgets(line, 128, stdin );
  sscanf(line, "%s %s", cmd, input);

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

  while (1)
  {
    sanitizePathname(pathname);
    //pwd(fd, running->cwd);
    getInput(cmd, pathname, fd, running);
    if (!strcmp(cmd, "test"))
    {
      // int testNum = 2;
      // printf("enter inode number you wish to look for:: ");
      // scanf("%d", &testNum);
      // //strcpy(name, getNameFromIno(fd, testNum));  //A test
      // char name[NAMELEN] = {'\0'};
      // getNameFromIno(fd, testNum, name);  //A test
      // printf("Name found = %s\n", name);
      int allocated_bno = balloc(fd);
      printf("Allocated block number %d\n", allocated_bno);
      int allocated_ino = ialloc(fd);
      printf("Allocated inode number %d\n", allocated_ino);

      idealloc(fd, allocated_ino);
      bdealloc(fd, allocated_bno);
    }
    else if (!strcmp(cmd, "pwd"))
      pwd(fd, running->cwd);
    else if (!strcmp(cmd, "ls"))
      ls(fd, running, pathname);
    else if (!strcmp(cmd, "cd"))
      cd(fd, running, pathname);
    else if (!strcmp(cmd, "quit"))
      quit();


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
