#ifndef PROJECT_H
#define PROJECT_H

#define TRUE 1
#define FALSE 0
#define DEBUG_STATUS 0



#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <stdint.h>


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

#define BLKSIZE  1024
#define NMINODE   100
#define NFD        16
#define NPROC       2

#define DEPTH 30
#define NAMELEN 256
#define STRLEN BLKSIZE
#define MAXINT 6291456


typedef struct minode{
  INODE inode;
  int dev, ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mountPtr;
}MINODE;

// open file table
typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  MINODE      *cwd;
  OFT         fd[NFD];
}PROC;




MINODE minode_table[NMINODE];        // global minode[ ] array

// found in HELPER.C
int get_block(int dev, int blk, char buf[ ]);
int put_block(int dev, int blk, char buf[ ]);
void init (PROC proc[NPROC], MINODE *root);
void mount_root(int dev, MINODE **root);
int get_itable_begin(int dev, GD *gp);
MINODE *iget(int dev, int ino);
void iput(MINODE *mip);
int getino(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
void checkMagicNumber(int fd);
void printSuperBlock(int fd);
void printInode(INODE* ipCur);
void fillItUp(int dev, PROC* running);
void fillItUp2(int dev, PROC* running);
void rpd2(int x, char numbuf[NAMELEN], char *location, int flag);
void addSingleEntryBlock(int dev, PROC* running);
void showMinode (void);

// found in DIR_TRAVERSE.C
void sanitizePathname(char pathname[DEPTH][NAMELEN]);
void parse(char input[STRLEN], char pathname[DEPTH][NAMELEN]);
int search (int dev, MINODE *mip, char *name);
int searchHelper(int dev, int level_indirection, int block_num, char *name);
MINODE* pathnameToMip(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
void ls(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
void ls2(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
void cd(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);

//found in MKDIR.C
void _mkdir(int dev, PROC* running, char pathname[DEPTH][NAMELEN]);
void my_mkdir(PROC *running, MINODE *pip, char *new_name);
int enter_name(MINODE *pip, int new_ino, char *new_name);
int mkFileHelper(int dev,int level_indirection, int *block_num, MINODE *pmip, int new_ideal_len, int new_name_len, char *new_name, int new_ino);
int enter_name_helper(MINODE *pip, int *i_block_ptr, int new_ideal_len, int new_name_len, char *new_name, int new_ino);
void pwd(int dev, MINODE* mip);
int pwdHelper(int dev, MINODE* mip, char pathname[DEPTH][NAMELEN]);
int getNameFromInoHelper(int dev, int level_indirection, int block_num, int ino, char fileName[NAMELEN]);
void getNameFromIno(int dev, int ino, char fileName[NAMELEN]);

//found alloc_dealloc.c
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);
int decFreeBlocks(int dev);
int incFreeBlocks(int dev);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int balloc(int dev);
int ialloc(int dev);
int bdealloc(int dev, int bno);
int idealloc(int dev, int ino);

// found MAIN.C
void quit(void);
void debugMode(char* fmt, ...);

// found in rmdir.c
void rmDir (int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
int isDirEmpty(int dev, MINODE* mip);
void rmEndFile(int dev, DIR* dp, DIR* prevdp, int block_num, char buf[BLKSIZE]);
void rmFileHelper(int dev, int level_indirection, int block_num, MINODE *pmip, MINODE *mip);
void freeBlockHelper(int dev, int level_indirection, int block_num);
void rmOnlyFile(int dev, MINODE *pmip, int *iblockToChange);
void rmMiddleFile(int dev, DIR *dp, int block_num, char buf[BLKSIZE]);
void findLastIblock(int dev, int level_indirection, int block_num, int* lastValidBlock);
void rmDirEntry(int dev, MINODE* pmip, MINODE* mip);


//LINK.C
void _link(int dev, PROC* running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE]);
void getChildFileName(char new_pathname_arr[DEPTH][NAMELEN], char new_filename[NAMELEN]);
void _symlink(int dev, PROC *running, char old_pathname[BLKSIZE], char new_pathname_arr[DEPTH][NAMELEN]);
void readLink (int dev, PROC* running, char sym_pathname_arr[DEPTH][NAMELEN]);
int readSymLink(int dev, PROC* running, MINODE *mip);
void _unlink(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);

// create .c
void create(int dev, PROC* running, char pathname[DEPTH][NAMELEN]);
void createHelper(PROC *running, MINODE *pmip, char *filename);

// chmod.c
void _chmod(int dev, PROC *running, char pathname[DEPTH][NAMELEN], char value[BLKSIZE]);

//open_close.c
void _truncate(MINODE *mip);
int open_file(int dev, PROC *running, char pathname[DEPTH][NAMELEN], char mode[BLKSIZE]);
int determineMode(char mode[BLKSIZE]);
void pfd(int dev, PROC* running);
void close_file(int dev, PROC *running, int fd_num);

//read_write.c
int bnoFromOffset(OFT *fd, int lbk);
int _read(OFT *fd, char *buf, int nbytes);
void printRead(PROC *running, int fd_num, int nBytes);
void _lseek(PROC *running, int fd_num, int position);
int _write(OFT *fd, char *buf, int nbytes);
void screen_write(PROC *running, int fd_num, char *buf);


//mv_cp.c
void mv(int dev, PROC *running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE]);
void copy(int dev, PROC *running, char newPathNameArray[DEPTH][NAMELEN], char oldPath[BLKSIZE]);

// cat.c
void cat (int dev, PROC *running, char pathname[DEPTH][NAMELEN]);


#endif
