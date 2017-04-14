#ifndef PROJECT_H
#define PROJECT_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <sys/types.h>
#include <unistd.h>

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
  OFT         *fd[NFD];
}PROC;


MINODE minode_table[NMINODE];        // global minode[ ] array

// found in HELPER.C(char*)lk, char buf[ ]);
void init (PROC proc[NPROC], MINODE *root);
void mount_root(int dev, MINODE **root);
int get_itable_begin(int dev, GD *gp);
MINODE *iget(int dev, int ino);
void iput(MINODE *mip);
int getino(int dev,PROC *running, char pathname[DEPTH][NAMELEN]);
void checkMagicNumber(int fd);
void printSuperBlock(int fd);
void printInode(INODE* ipCur);

// found in DIR_TRAVERSE.C
void sanitizePathname(char pathname[DEPTH][NAMELEN]);
void parse(char input[STRLEN], char pathname[DEPTH][NAMELEN]);
int search (int dev, MINODE *mip, char *name);
int searchHelper(int dev, int level_indirection, int block_num, int inode_table_index);
void ls(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
void cd(int dev, PROC *running, char pathname[DEPTH][NAMELEN]);

// int pwd (int dev, PROC *running, char pathname[DEPTH][NAMELEN]);
// int pwdHelper(int dev, char pathname[DEPTH][NAMELEN], MINODE *mip);
void pwd(int dev, MINODE* mip);
void pwdHelper(int dev, MINODE* mip);
int getNameFromInoHelper(int dev, int level_indirection, int block_num, int ino, char fileName[NAMELEN]);
void getNameFromIno(int dev, int ino, char fileName[NAMELEN]);

//found alloc_dealloc.c
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);
int decFreeBlocks(int dev);
int incFreeBlocks(int dev);
int balloc(int dev);
int ialloc(int dev);
int bdealloc(int dev, int bno);
int idealloc(int dev, int ino);
// found MAIN.C
void quit(void);

#endif
