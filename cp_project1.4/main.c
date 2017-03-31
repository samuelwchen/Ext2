#include "project.h"

void getInput(char cmd[64], char pathname[DEPTH][NAMELEN])
{
  //char cmd[64], pathname[DEPTH][NAMELEN];
  char input[BLKSIZE];
  char line[128];
  char *token;
  char delim[2] = " ";

  for (int i = 0; i < DEPTH; i++)
  {
    strcpy(pathname[i], "\0" );
  }

  printf("-------------------------------------------\n");
  //GET INPUT
  printf("Enter command [pathname]: ");
  //scanf("%s", input);
  fgets(line, 128, stdin );
  sscanf(line, "%s %s", cmd, input);
// printf("path = %s\n", input);
// printf("press any key\n");
// getchar();
  // token = strtok(input, delim);
  // strcpy(cmd, token);
  //parse input into pathname
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

  //may need to change later,
  //Need to SET FIRST PROCCESS and point running at it
  proc[0].cwd = root;
  //what do we do about the pid and next and etc?
  running = &proc[0];

  MINODE *mip = iget(fd, 2);
  printInode(&(mip->inode));
  //
  // //GET INPUT
  // printf("Enter pathname ");
  // scanf("%s", input);
  //
  // printf("You entered: %s\n", input);
  // parse(input, pathname);
  //
  // for (int i = 0; pathname[i][0] != '\0'; i++)
  // {
  //   printf("pathname[%d] = %s\n", i, pathname[i]);
  // }

  //NOTE: Will we need to change fd to running->cwd->dev at some point?
  // int tempino = getino(fd, running, pathname);
  // printf("Back in main, ino = %d!\n", tempino);
  // if (tempino != 0)
  //   printInode( &(iget(fd, tempino)->inode) );
  //search(fd, root, pathname[1]);
  // printf("%d of minode array dirty", minode_table[12].dirty);
  // printf("hello my FAVORITEST michelle\n");
/*
  ls(fd, running, pathname);

  //GET INPUT
  printf("Enter pathname ");
  scanf("%s", input);

  printf("You entered: %s\n", input);
  parse(input, pathname);
  cd(fd, running, pathname);
*/
  // //GET INPUT
  // printf("Enter pathname ");
  // scanf("%s", input);
  //
  // printf("You entered: %s\n", input);
  // parse(input, pathname);
//enter quit when done
while (1)
{
  getInput(cmd, pathname);
  if (!strcmp(cmd, "ls"))
    ls(fd, running, pathname);
  if (!strcmp(cmd, "cd"))
    cd(fd, running, pathname);
  if (!strcmp(cmd, "quit"))
    quit();
}
  // getInput(cmd, pathname);
  // if (!strcmp(cmd, "ls"))
  //   ls(fd, running, pathname);
  // if (!strcmp(cmd, "cd"))
  //   cd(fd, running, pathname);
  // if (!strcmp(cmd, "quit"))
  //   quit();

  //ls(fd, running, pathname);

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
