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

#define stat xv6_stat  
#include "./types.h"
#include "./fs.h"
#include "./stat.h"
#undef stat
#undef dirent
#define BLOCK_SIZE (BSIZE)

char* addr;
char bitarr[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};	//filesystem image address
#define CHECKBIT(bitmap, blocknum) ((*(bitmap + blocknum/8)) & (bitarr[blocknum%8]))
//set the bit
void setbit (char* bitmap, uint blocknum) {
  bitmap = (bitmap + blocknum/8);
  *bitmap = (*bitmap) | (bitarr[blocknum%8]);
}
//unset the bit
void unsetbit (char* bitmap, uint blocknum) {
  bitmap = (bitmap + blocknum/8);
  *bitmap = (*bitmap) & (~bitarr[blocknum%8]);
}
// check the directory for inconsistencies
void checkdir(struct dinode* dip, uint ino,  char* inodebitmap, char* inodebitmap_cp, short* refcounts) {  
  int n = dip[ino].size/sizeof(struct dirent);  
  int entry_num = BLOCK_SIZE/sizeof(struct dirent);
  struct dirent* de;
  int i;
  // pdir represents '.'  gdir represents '..'
  int pdir = 0;  
  int gpdir = 0;
  int counter = 0;
  // indirect block address
  uint* inaddr = (uint *)(addr + (dip[ino].addrs[NDIRECT] * BLOCK_SIZE));
  while (counter < NDIRECT + NINDIRECT) {
   if (counter < NDIRECT)    //entries in blocks pointed direct addresses 
     de = (struct dirent*)(addr + dip[ino].addrs[counter] * BLOCK_SIZE);
   else {     				//~~~indirect addresses
     de = (struct dirent*)(addr + (*inaddr * BLOCK_SIZE));
     inaddr++;
   }
   int limit = entry_num;
   if (n < entry_num) limit = n;    
    for (i = 0; i < limit; i++, de++) {
      if (de->inum != 0) {
        if (!CHECKBIT(inodebitmap, de->inum)){
          fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
          exit(1);
        }          
        else {
          if (strcmp(de->name, ".") == 0) {
	          pdir = 1;
	          if (de->inum != ino) {
              fprintf(stderr,"ERROR: directory not properly formatted.\n");
              exit(1);
            }	            
	        }  
          else if (strcmp(de->name, "..")==0) {
	          if (ino == ROOTINO) {
	            if (de->inum != ino) {
                fprintf(stderr,"ERROR: root directory does not exist.\n");
                exit(1);
              }	               
	          }
	        gpdir = 1;
	        }
          else if (dip[de->inum].type==T_DIR) {
	          if (CHECKBIT(inodebitmap_cp, de->inum))
              unsetbit(inodebitmap_cp, de->inum);
            else{
              fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
              exit(1);
            }              
          }
          else if (dip[de->inum].type == T_FILE && strcmp(de->name, "") != 0)
	          refcounts[de->inum]++;
          } 
          unsetbit(inodebitmap_cp, de->inum);
      }
    }
    if (n <= entry_num) break;
    counter++;
    n -= entry_num;
  }
  if (!pdir || !gpdir){
    fprintf(stderr,"ERROR: directory not properly formatted.\n");
    exit(1);
  }    
}

int main(int argc, char* argv[]) {
  int fsfd,i;    
  struct stat filestat;
  char *bitmap,*inodebitmap; 
  short *refcounts;
  struct dinode* dip;
  struct superblock* sb;

  if (argc < 2){
    fprintf(stderr,"Usage: fcheck <file_system_image>\n");
    exit(1);
  }     
                
  fsfd = open(argv[1], O_RDONLY);
  if (fsfd == -1) {    
    exit(1);
  }
  
  if (fstat(fsfd, &filestat) == -1) {    
    exit(1);
  }    
  
  addr = mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
  close(fsfd);
  
  if (addr == MAP_FAILED) {
    fprintf(stderr,"mmap failed.\n");
    exit(1);
  }    
  
  // read the super block
  sb = (struct superblock*) (addr + (1 * BLOCK_SIZE));
  
  int bitblocks = sb->size/(BLOCK_SIZE * 8) + 1;
  int metablocks = sb->ninodes/IPB + 3 + bitblocks;  
  dip = (struct dinode*) (addr + IBLOCK((uint)0)*BLOCK_SIZE);
  // bitmap for all the blocks in the image 
  bitmap = (char *)(addr + (IBLOCK((uint)0) * BLOCK_SIZE) + (sb->ninodes/IPB + 1) * BLOCK_SIZE);
  char* bitmap_cp;
  bitmap_cp = (char *) malloc(bitblocks * BLOCK_SIZE);
  memcpy(bitmap_cp, bitmap, bitblocks * BLOCK_SIZE);  
  
  inodebitmap = (char *) malloc ((sb->ninodes/8) + 1);
  refcounts = (short *) malloc (sb->ninodes * sizeof(short));  
  for (i = 0; i < sb->ninodes; i++) {
    refcounts[i] = 0;
    if (dip[i].type == 0) unsetbit(inodebitmap, i);
    else setbit(inodebitmap, i);
  }
  char* inodebitmap_cp = (char *) malloc ((sb->ninodes/8)+1);
  memcpy(inodebitmap_cp, inodebitmap, (sb->ninodes/8)+1);
    
  for (i = 0; i < sb->ninodes; i++) {     
    if (dip[i].type < 0 || dip[i].type > 3) {      
      fprintf(stderr,"ERROR: bad inode.\n");
      exit(1);
    } 
    
    else {
      int j;      
      for (j = 0; j < NDIRECT+1; j++) {
        if (dip[i].addrs[j] != 0) {
	        if (dip[i].addrs[j] < metablocks || dip[i].addrs[j] >= sb->size) {
	          if (j == NDIRECT) {
              fprintf(stderr,"ERROR: bad indirect address in inode.\n");
              exit(1);
            } else{
              fprintf(stderr,"ERROR: bad direct address in inode.\n");
              exit(1);
            }	            	          
	        } 
	        
	        else {
	          if (!CHECKBIT(bitmap, dip[i].addrs[j])){
              fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n"); 
              exit(1);
            }	            
	          else {
	            if (CHECKBIT(bitmap_cp, dip[i].addrs[j]))
	              unsetbit(bitmap_cp, dip[i].addrs[j]); 
	            else{
                fprintf(stderr,"ERROR: direct address used more than once.\n");
                exit(1);
              }		            
	          }
	        }    
	      }
      }
      
      if (dip[i].addrs[NDIRECT] != 0) {
	      int k;
        uint* inaddr = (uint *)(addr + (dip[i].addrs[NDIRECT] * BLOCK_SIZE));
        for (k = 0; k < NINDIRECT; k++, inaddr++) {
          if (*inaddr != 0) {
          if (*inaddr < metablocks || *inaddr > sb->size){
            fprintf(stderr,"ERROR: bad indirect address in inode.\n");
            exit(1);
          }                     
          else {
            if (!CHECKBIT(bitmap, *inaddr)){
              fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
              exit(1);
            }              
            else {
              if (CHECKBIT(bitmap_cp, *inaddr))
                unsetbit(bitmap_cp, *inaddr);
              else{
                fprintf(stderr,"ERROR: indirect address used more than once.\n");
                exit(1);
              }                
            }
          }
	      }
      }
    }    
    if (i == ROOTINO) {
    	//when it's ROOTINO 
      if (dip[i].type != T_DIR) {//check directory
        fprintf(stderr,"ERROR: root directory does not exist.\n");
        exit(1);
      }        
    }    
    if (dip[i].type == T_DIR)
    //check consistency
      checkdir(dip,(uint)i,  inodebitmap, inodebitmap_cp, refcounts);
    }
  }  
    // check the inconsistencies of bitmap
  for (i = metablocks; i < sb->size; i++) {
    if (CHECKBIT(bitmap_cp, i)) {
      fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
      exit(1);
    }      
  }
  // check the inconsistencies of inode bitmap 
  for (i = ROOTINO; i < sb->ninodes; i++) {
    if (CHECKBIT(inodebitmap, i)) {
      if (CHECKBIT(inodebitmap_cp, i)) {
        fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
        exit(1);
      }        
    }    
    if (dip[i].type == T_FILE) {
      if (dip[i].nlink != refcounts[i]){
      	// check the reference count
        fprintf(stderr,"ERROR: bad reference count for file.\n");
        exit(1);
      }        
    }
  }
  return 0;
}
