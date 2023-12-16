#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#include "types.h"
#include "fs.h"

#define T_FILE 2 // Regular file
#define T_DIR 1  // Directory
#define T_DEV 3  // Device file

#define BLOCK_SIZE (BSIZE)
char bitarr[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}; // filesystem image address
#define CHECKBIT(databitmap, blocknum) ((*(databitmap + blocknum / 8)) & (bitarr[blocknum % 8]))

#define ENTRY_NUM (BSIZE / sizeof(struct dirent))
void setbit(char *bitmap, uint blocknum)
{
  bitmap = (bitmap + blocknum / 8);
  *bitmap = (*bitmap) | (bitarr[blocknum % 8]);
} // gpt
// unset the bit
void unsetbit(char *bitmap, uint blocknum)
{
  bitmap = (bitmap + blocknum / 8);
  *bitmap = (*bitmap) & (~bitarr[blocknum % 8]);
} // gpt
void check_inode_type(char *addr, struct dinode *inodes, int num_inodes);                                        //  1
void check_inode_addr(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, int metablocks); // 2
// void check_root_addr(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *inodebitmap);
void check_root_path(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *inodebitmap, char *inodeblocks); // 3 4

void check_bitmap_consistency_5(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, int metablocks);                                                               // 5
void check_bitmap_consistency_6(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks);                                        // 6
void check_direct_redundent_7(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks);                                          // 7
void check_indirect_redundent_8(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks);                                        // 8
void check_inode_not_exist(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap);  // 9
void check_wrong_free_inode(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap); // 10
void check_reference_count(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap);  // 11
void check_dir_redundent(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap);    // 12
int main(int argc, char *argv[])
{
  int i, n, fsfd;
  struct stat filestat;
  char *addr;
  char *databitmap;
  char *inodebitmap;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;
  char *inodeblocks;
  if (argc < 2)
  {
    fprintf(stderr, "Usage: sample fs.img ...\n");
    exit(1);
  }

  fsfd = open(argv[1], O_RDONLY);
  if (fstat(fsfd, &filestat) < 0)
  {
    perror(argv[1]);
    exit(1);
  }

  /* Dont hard code the size of file. Use fstat to get the size */
  addr = mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);

  if (addr == MAP_FAILED)
  {
    perror("mmap failed");
    exit(1);
  }
  /* read the super block */
  sb = (struct superblock *)(addr + 1 * BLOCK_SIZE);
  // printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);
  /* read the inodes */
  int bitblocks = sb->size / (BLOCK_SIZE * 8) + 1;
  int metablocks = sb->ninodes / IPB + 3 + bitblocks;
  inodeblocks = (char *)(addr + 2 * BLOCK_SIZE);
  inodebitmap = (char *)malloc((sb->ninodes / 8) + 1);
  databitmap = (char *)(addr + (IBLOCK((uint)0) * BLOCK_SIZE) + (sb->ninodes / IPB + 1) * BLOCK_SIZE);
  dip = (struct dinode *)(addr + IBLOCK((uint)0) * BLOCK_SIZE);
  // printf("begin addr %p, begin inode %p , offset %d \n", addr, dip, (char *)dip -addr);
  char *databitmap_temp = calloc(bitblocks, BLOCK_SIZE);
  memcpy(databitmap_temp, databitmap, bitblocks * BLOCK_SIZE);
  // read root inode
  printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // get the address of root dir
  // root dir need modification, we need traversal all inode to find rood idrectory;
  de = (struct dirent *)(addr + (dip[ROOTINO].addrs[0]) * BLOCK_SIZE);

  // print the entries in the first block of root dir

  n = dip[ROOTINO].size / sizeof(struct dirent);
  for (i = 0; i < n; i++, de++)
  {
    // printf(" inum %d, name %s ", de->inum, de->name);
    // printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
  }
  int *ref_bit = (int *)malloc(sb->ninodes * sizeof(int));
  for (i = 0; i < sb->ninodes; i++)
  {
    ref_bit[i] = 0;
    if (dip[i].type == 0)
      unsetbit(inodebitmap, i);
    else
      setbit(inodebitmap, i);
  }
  char *inodebitmap_temp = calloc(1, (sb->ninodes / 8) + 1);
  memcpy(inodebitmap_temp, inodebitmap, (sb->ninodes / 8) + 1);
  check_inode_type(addr, dip, sb->ninodes);                 // 1
  check_inode_addr(addr, sb, dip, sb->ninodes, metablocks); // 2
  // check_root_addr(addr, sb, dip, sb->ninodes, inodebitmap);                                                                  // 3
  check_root_path(addr, sb, dip, sb->ninodes, inodebitmap, inodeblocks);                                                      // 4
  check_bitmap_consistency_5(addr, sb, dip, sb->ninodes, databitmap, metablocks);                                             // 5
  check_bitmap_consistency_6(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks);                            // 6
  check_direct_redundent_7(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks);                              // 7
  check_indirect_redundent_8(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks);                            // 8
  check_inode_not_exist(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks, inodebitmap_temp, inodebitmap);  // 9
  check_wrong_free_inode(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks, inodebitmap_temp, inodebitmap); // 10
  check_reference_count(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks, inodebitmap_temp, inodebitmap);  // 11
  check_dir_redundent(addr, sb, dip, sb->ninodes, databitmap, databitmap_temp, metablocks, inodebitmap_temp, inodebitmap);

  exit(0);
}
// requirement 1
void check_inode_type(char *addr, struct dinode *inodes, int num_inodes)
{
  int i;
  int count = 0;
  int total = 0;
  for (i = 0; i < num_inodes; i++)
  {
    total++;
    if (inodes[i].type != T_FILE && inodes[i].type != T_DIR && inodes[i].type != T_DEV)
    {
      if (inodes[i].type == 0)
      {
        count++;
        continue;
      }
      printf("ERROR: bad inode.\n");

      exit(1);
      // Additional error handling if required
    }
  }
}
// check requirement 2 about inode.
void check_inode_addr(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, int metablocks)
{
  int i = 0;
  int j = 0;
  int k = 0;
  for (i = 0; i < num_inodes; i++)
  {
    // struct dinode *inode = &inodes[i];

    for (j = 0; j < NDIRECT + 1; j++)
    {
      if (inodes[i].type >= 0 && inodes[i].type <= 3)
      {
        if ((inodes[i].addrs[j] < metablocks || inodes[i].addrs[j] >= sb->size) && inodes[i].addrs[j] != 0)
        {
          if (j == NDIRECT)
          {
            printf("ERROR: bad indirect address in inode.\n");
            exit(1);
          }
          else
          {
            printf("ERROR: bad direct address in inode.\n");
            exit(1);
          }
        }

        if (inodes[i].addrs[NDIRECT] != 0)
        {
          uint *inaddr = (uint *)(addr + (inodes[i].addrs[NDIRECT] * BLOCK_SIZE));
          for (k = 0; k < NDIRECT; k++)
          {
            if (*inaddr != 0)
            {
              if (*inaddr < metablocks || *inaddr >= sb->size)
              {
                printf("ERROR: bad indirect address in inode.\n");
                exit(1);
              }
            }
            inaddr++;
          }
        }
      }
    }
  }
}
// check root path .. and ., 4 and 3
void check_root_path(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *inodebitmap, char *inodeblocks)
{
  bool cur_path = false;
  bool par_path = false;
  // int entry = BLOCK_SIZE / sizeof(struct dirent);
  int i = 0;
  int j = 0;
  int k = 0;
  struct dirent *de;
  // int loop;
  struct dinode *inode;
  uint inaddr;
  inode = (struct dinode *)(inodeblocks);
  for (i = 0; i < sb->ninodes; i++, inode++)
  {
    if (i == ROOTINO)
    {
      if (inode->type != T_DIR)
      {
        printf("ERROR: root directory does not exist.\n");
        exit(1);
      }
      else
      {
        for (j = 0; j < NDIRECT; j++)
        {
          inaddr = inode->addrs[j];
          if (inaddr != 0)
          {
            de = (struct dirent *)(addr + inaddr * BLOCK_SIZE);
            for (k = 0; k < ENTRY_NUM; k++, de++)
            {
              if (strcmp(".", de->name) == 0)
              {
                cur_path = true;
                if (de->inum != 1)
                {
                  fprintf(stderr, "ERROR: directory not properly formatted.\n");
                  exit(1);
                }
              }
              else if (strcmp("..", de->name) == 0)
              {
                par_path = true;
                if ((i != 1 && de->inum == i) || (i == 1 && de->inum != i))
                {
                  fprintf(stderr, "ERROR: root directory does not exist.\n");
                  exit(1);
                }
              }
              if (par_path && cur_path)
              {
                break;
              }
            }
          }
          if (par_path && cur_path)
            break;
        }
        if (!par_path || !cur_path)
        {
          fprintf(stderr, "ERROR: directory not properly formatted.2\n");
          exit(1);
        }
      }
    }
  }
  cur_path = false;
  par_path = false;
  inode = (struct dinode *)(inodeblocks);
  for (i = 0; i < sb->ninodes; i++, inode++)
  {
    if (i != ROOTINO && inode->type == T_DIR)
    {
      if (inode->type != T_DIR)
      {
        printf("ERROR: root directory does not exist.\n");
        exit(1);
      }
      else
      {
        for (j = 0; j < NDIRECT; j++)
        {
          inaddr = inode->addrs[j];
          if (inaddr != 0)
          {
            de = (struct dirent *)(addr + inaddr * BLOCK_SIZE);
            for (k = 0; k < ENTRY_NUM; k++, de++)
            {
              if (strcmp(".", de->name) == 0)
              {
                cur_path = true;
                if (de->inum != i)
                {
                  fprintf(stderr, "ERROR: directory not properly formatted.3\n");
                  exit(1);
                }
              }
              else if (strcmp("..", de->name) == 0)
              {
                par_path = true;
                if ((i != 1 && de->inum == i) || (i == 1 && de->inum != i))
                {
                  fprintf(stderr, "ERROR: root directory does not exist.\n");
                  exit(1);
                }
              }
              if (par_path && cur_path)
              {
                break;
              }
            }
          }
          if (par_path && cur_path)
            break;
        }
        if (!par_path || !cur_path)
        {
          fprintf(stderr, "ERROR: directory not properly formatted.4\n");
          exit(1);
        }
      }
    }
  }
}
// check 5, bitmap consistancy
void check_bitmap_consistency_5(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, int metablocks)
{
  int i = 0;
  int j = 0;
  for (i = 0; i < sb->ninodes + 1; i++)
  {
    if (inodes[i].type >= 0 && inodes[i].type <= 3)
    {
      for (j = 0; j < NDIRECT + 1; j++)
      {
        if (inodes[i].addrs[j] == 0)
        {
          if (inodes[i].addrs[j] >= metablocks && inodes[i].addrs[j] < sb->size)
          {
            if (!CHECKBIT(databitmap, inodes[i].addrs[j]))
            {
              printf("ERROR: address used by inode but marked free in bitmap.\n");
              exit(1);
            }
          }
        }
      }
    }
  }
}
void check_bitmap_consistency_6(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks)
{
  int i = 0;
  for (i = metablocks; i < sb->size; i++)
  {
    if (CHECKBIT(databitmap_temp, i))
    {
      fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
      exit(1);
    }
  }
}
void check_direct_redundent_7(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks)
{
  int i = 0;
  int j = 0;
  int k = 0;
  // uint *inaddr;
  for (i = 0; i < sb->ninodes; i++)
  {
    if (inodes[i].type >= 0 && inodes[i].type <= 3)
    {
      for (j = 0; j < NDIRECT + 1; j++)
      {
        if (inodes[i].addrs[j] != 0)
        {
          if (inodes[i].addrs[j] >= metablocks && inodes[i].addrs[j] < sb->size)
          {
            if (CHECKBIT(databitmap, inodes[i].addrs[j]))
            {
              if (CHECKBIT(databitmap_temp, inodes[i].addrs[j]))
              {
                unsetbit(databitmap_temp, inodes[i].addrs[j]);
              }
              else{
                printf("ERROR: direct address used more than once.\n");
                exit(1);
              }
            }
          }
        }
      }
      if (inodes[i].addrs[NDIRECT] != 0)
      {
        uint *inaddr = (uint *)(addr + (inodes[i].addrs[NDIRECT] * BLOCK_SIZE));
        for (k = 0; k<NINDIRECT;k++,inaddr++){
          if(*inaddr >=metablocks && *inaddr<=sb->size){
            if(CHECKBIT(databitmap_temp, *inaddr)){
              unsetbit(databitmap,*inaddr);
            }
          }
        }
      }
    }
  }
}

void check_indirect_redundent_8(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks)
{ // 8
  int i = 0;
  int j = 0;
  int k = 0;
  // uint *inaddr;
  for (i = 0; i < sb->ninodes; i++)
  {
    if (inodes[i].type >= 0 && inodes[i].type <= 3)
    {
      for (j = 0; j < NDIRECT + 1; j++)
      {
        if (inodes[i].addrs[j] != 0)
        {
          if (inodes[i].addrs[j] >= metablocks && inodes[i].addrs[j] < sb->size)
          {
            if (CHECKBIT(databitmap, inodes[i].addrs[j]))
            {
              if (CHECKBIT(databitmap_temp, inodes[i].addrs[j]))
              {
                unsetbit(databitmap_temp, inodes[i].addrs[j]);
              }
            }
          }
        }
      }
      if (inodes[i].addrs[NDIRECT] != 0)
      {
        uint *inaddr = (uint *)(addr + (inodes[i].addrs[NDIRECT] * BLOCK_SIZE));
        for (k = 0; k<NINDIRECT;k++,inaddr++){
          if(*inaddr >=metablocks && *inaddr<=sb->size){
            if(CHECKBIT(databitmap_temp, *inaddr)){
              unsetbit(databitmap,*inaddr);
            }
            else{
              printf("ERROR: indirect address used more than once.\n");
              exit(1);
            }
          }
        }
      }
    }
  }
}

void check_inode_not_exist(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap) // 9
{
  int i = 0;
  for (i = ROOTINO; i < sb->ninodes; i++)
  {
    if (CHECKBIT(inodebitmap, i))
    {
      if (CHECKBIT(inodebitmap_temp, i))
      {
        printf("ERROR: inode marked use but not found in a directory.\n");
      }
    }
  }
}
void check_wrong_free_inode(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap) // 10
{
  int i = 0;
  int j = 0;
  int k = 0;
  struct dirent *de;
  for (i = 0; i < sb->ninodes; i++)
  {
    for (j = 0; j < (NDIRECT + NINDIRECT); j++)
    {
      if (j < NDIRECT)
      {
        de = (struct dirent *)(addr + (inodes[i].addrs[j]) * BLOCK_SIZE);
      }
      else
      {
        de = (struct dirent *)(addr + *((uint *)(addr + inodes[i].addrs[NDIRECT] * BLOCK_SIZE)) * BLOCK_SIZE);
      }
      for (k = 0; k < (BLOCK_SIZE / sizeof(struct dirent)) || k < inodes[i].size / sizeof(struct dirent); k++)
      {
        if (de->inum != 0)
        {
          if (!CHECKBIT(inodebitmap, de->inum))
          {
            printf("ERROR: inode referred to in directory but marked free.\n");
            exit(1);
          }
        }
      }
    }
  }
}
void check_reference_count(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap) // 11
{
  short *reference_count;
  reference_count = (short *)malloc(sb->ninodes * sizeof(short));
  int i = 0;
  // int j = 0;
  for (i = 0; i < sb->ninodes; i++)
  {
    reference_count[i] = 0;
    // if(inodes[i].type == 0){
    //   inodebitmap = (inodebitmap + i/8);
    //   *inodebitmap = (*inodebitmap) & (~bitarr[i%8]);
    //   }
    // else{
    //     inodebitmap = (inodebitmap + i/8);
    //     *inodebitmap = (*inodebitmap) | (bitarr[i%8]);
    // }
  }
  for (i = 0; i < sb->ninodes; i++)
  {
    if (inodes[i].type == T_FILE && inodes[i].nlink != reference_count[i])
    {
      printf("ERROR: bad reference count for file.\n");
    }
  }
}
void check_dir_redundent(char *addr, struct superblock *sb, struct dinode *inodes, int num_inodes, char *databitmap, char *databitmap_temp, int metablocks, char *inodebitmap_temp, char *inodebitmap) // 12
{
  int i = 0;
  int j = 0;
  int k = 0;
  struct dirent *de;
  for (i = 0; i < sb->ninodes; i++)
  {
    for (j = 0; j < (NDIRECT + NINDIRECT); j++)
    {
      if (j < NDIRECT)
      {
        de = (struct dirent *)(addr + (inodes[i].addrs[j]) * BLOCK_SIZE);
      }
      else
      {
        de = (struct dirent *)(addr + *((uint *)(addr + inodes[i].addrs[NDIRECT] * BLOCK_SIZE)) * BLOCK_SIZE);
      }
      for (k = 0; k < (BLOCK_SIZE / sizeof(struct dirent)) || k < inodes[i].size / sizeof(struct dirent); k++)
      {
        if (de->inum != 0)
        {
          if (CHECKBIT(inodebitmap, de->inum))
          {
            if (strcmp(de->name, "..") != 0 && strcmp(de->name, ".") != 0)
            {
              if (inodes[de->inum].type == T_DIR)
              {
                if (!CHECKBIT(inodebitmap_temp, de->inum))
                {
                  printf("ERROR: directory appears more than once in file system.\n");
                }
                else
                {
                  inodebitmap = (inodebitmap + i / 8);
                  *inodebitmap = (*inodebitmap) & (~bitarr[i % 8]);
                }
              }
            }
          }
        }
        de++;
      }
    }
  }
}
