#include "project.h"

void getInput(char cmd[64], char pathname[DEPTH][NAMELEN])
{
  //char cmd[64], pathname[DEPTH][NAMELEN];
  char input[BLKSIZE] = {'\0'};
  char line[128] = {'\0'};
  char *token = NULL;
  char delim[2] = " ";

  sanitizePathname(pathname);

  printf("-------------------------------------------\n");

  //GET INPUT
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
    getInput(cmd, pathname);
    if (!strcmp(cmd, "ls"))
      ls(fd, running, pathname);
    if (!strcmp(cmd, "cd"))
      cd(fd, running, pathname);
    if (!strcmp(cmd, "quit"))
      quit();
  }

  quit();

}


void quit(void)
{
  printf("quit()\n");
  for (int i = 0; i < NMINODE; i++)
  {
    if (minode_table[i].refCount != 0)
      iput(&minode_table[i]);
  }

  exit(0);  // terminate program
}
