#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

/*
 * disk image
 */
unsigned char *disk;

/*
 * super block
 */
struct ext2_super_block *sb;

/*
 * group desc
 */
struct ext2_group_desc *gd;

/*
 * pointer to all inodes
 */
char *inodes;

/*
 * pointer to block map
 */
unsigned char * block_map;

/*
 * pointer to inode map
 */
unsigned char * inode_map;

/*
 * a dir entry size except name field
 */
int dir_size_except_name = sizeof(unsigned int)+sizeof(
  unsigned short)+sizeof(unsigned char)*2;

/*
 * Calculates the total size of current entry given
 * the name length.
 */
#define entry_size(x) (((x*sizeof(char) - 1) / 4 + 1) * 4 + dir_size_except_name)

/*
 * Checks the x th bit(start from 0) of block bitmap.
 */
#define check_block_bitmap(x)(*(block_map+((x)/8))>>((x)%8) & 0x1)

/*
 * Checks the x th bit(start from 0) of inode bitmap.
 */
#define check_inode_bitmap(x)(*(inode_map+((x)/8))>>((x)%8) & 0x1)

// convert inode's i_mode to file type
unsigned char get_type(struct ext2_inode* inode){
  unsigned char type = EXT2_FT_UNKNOWN;
  if((inode -> i_mode) & EXT2_S_IFLNK)
           type = EXT2_FT_SYMLINK;
     if((inode -> i_mode) & EXT2_S_IFREG)
           type = EXT2_FT_REG_FILE;
     if((inode -> i_mode) & EXT2_S_IFDIR)
           type = EXT2_FT_DIR;
    return type;
}
/*
 *return the inode corresponding to its inode number.
 */
struct ext2_inode* get_inode(unsigned int num){
  if(num <= 0){
    return NULL;
  }
  struct ext2_inode *inode_tb = (struct ext2_inode *)(disk + gd->bg_inode_table*EXT2_BLOCK_SIZE);
  struct ext2_inode *e_inode = (struct ext2_inode*)(inode_tb + (num-1));
  return e_inode;
}

/*
 * check the bitmap, if type = 0, check block bitmap
 * if type = 1, check inode bitmap 
 */
int check_bitmap(int num, int type){
  int result = 0;
  int buf = num / 8;
    int offset = num % 8;
    if(offset == 0){
    offset = 8;
    buf = buf -1;
  }
    if(type == 1){
      if(!(inode_map[buf] >> (offset - 1) & 0x1))
        result = 1;
    }
    else if (type == 0){
      if(!(block_map[buf] >> (offset -1) & 0x1))
        result = 1;
    }
    return result;
}
/*
 * function to malloc memory space
 * with error checking.
 */
void* safe_malloc(int size){
  void* result = malloc(size);
  if(result == NULL){
    errno = ENOMEM;
    perror("malloc");
    exit(errno);
  }
  return result;
}

/*
 * Initializes the ext2.
 * disk_path is the path to the image file.
 */
int mounting_ext2(char *disk_path){
	int fd = open(disk_path, O_RDWR);
	disk = mmap(
    	NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
        return 1;
    }
    // sets superblock
    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    // sets groupdesc
    gd = (struct ext2_group_desc *)(disk + 2*EXT2_BLOCK_SIZE);
    // sets inodes
    inodes = (char *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    // sets blocks bitmap
    block_map = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
    // sets inode bitmap
    inode_map = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
    return 0;
}


/*
 * Return the pointer of the inode of the directory of given path.
 * If given path doesn't exists, return NULL.
 */
struct ext2_inode *dir_inode(char *path){
	if(path[strlen(path)-1]!='/'){
		fprintf(stderr,
			"dir_inode: Please enter a path to a directory ending with /.\n");
    exit(1);
		}
	if(path[0]!='/'){
		fprintf(stderr,
			"dir_inode: Please enter the absolute path.\n");
    exit(1);
		}
	// let cur_inode points to root inode
	struct ext2_inode *cur_inode = (struct ext2_inode *)(
		inodes+(EXT2_ROOT_INO - 1) * sizeof(struct ext2_inode));
	if (strlen(path)==1){
		return cur_inode;
	}
	int position = 1;
	int cur_start = 0;
	char *dir_to_find = malloc(EXT2_NAME_LEN*sizeof(char));
	int j;
	while(1){
		if(path[position]!='/'){
			position++;
		}else{
			strncpy(dir_to_find,
				&(path[cur_start+1]),
				(position-cur_start-1)*sizeof(char));
			dir_to_find[position-cur_start-1]='\0';
			// loops thorugh all blocks of current directory
			for (j = 0; j < cur_inode->i_blocks/2; j++) {
          		unsigned int curBlock = cur_inode->i_block[j];
          		int p = 0;
          		struct ext2_dir_entry * de = (
          			struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
          		// loops through current block
         		while (p < EXT2_BLOCK_SIZE) {
            		char * dir_name = safe_malloc(de->name_len+1);
            		strncpy(dir_name, de->name, de->name_len);
            		dir_name[de->name_len]='\0';
					      if((de->inode != 0) &&
                  ((unsigned int) de->file_type == EXT2_FT_DIR) &&
						      (strcmp(dir_name,dir_to_find)==0)) {
              			 cur_inode = (struct ext2_inode *)(
              			 	inodes+(de->inode - 1) * sizeof(struct ext2_inode));
              			if(position == strlen(path)-1){
              				free(dir_name);
              				free(dir_to_find);
              				return cur_inode;
              			}
              			free(dir_name);
              			goto next_dir;
            		}
            		// sets de to next dir entry and p to current total length
            		p += de->rec_len;
            		de = (void *) de + de->rec_len;
            		free(dir_name);
          		}
        	}
        	free(dir_to_find);
        	return NULL;
        	next_dir: cur_start = position;
            position = cur_start+1;
		}
	}
}

/*
 * Returns the inode index (starts from 1)
 * of the file in dir of given inode of dir dir_inode.
 * If no files with given name in current directory, returns 0.
 */
unsigned int find_file(char *name,unsigned char file_type,
  struct ext2_inode *dir_inode){
  int j;
  // loops through all blocks of current directory
  for (j = 0; j < dir_inode->i_blocks/2; j++) {
    unsigned int curBlock = dir_inode->i_block[j];
    int p = 0;
    struct ext2_dir_entry * de = (
    struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
    // loops through current block
    while (p < EXT2_BLOCK_SIZE) {
      if((de->file_type == file_type) && (strlen(name) == de->name_len) &&
        (strncmp(name,de->name,de->name_len)==0) && 
        (de->inode != 0)){
        return de->inode;
      }
      // sets de to next dir entry and p to current total length
      p += de->rec_len;
      de = (void *) de + de->rec_len;
    }
  }
  return 0;
}

/*
 * Removes the directory entry of a
 * file or alink(indicated by file_type) and 
 * returns the inode number (starts from 1)
 * of the file in dir of given inode of dir dir_inode.
 * If no files with given name in current directory, returns 0.
 */
unsigned int remove_entry(char *name,
  struct ext2_inode *dir_inode){
  int j;
  // loops through all blocks of current directory
  for (j = 0; j < dir_inode->i_blocks/2; j++) {
    unsigned int curBlock = dir_inode->i_block[j];
    int p = 0;
    struct ext2_dir_entry * de = (
    struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
    // if the first entry in current block is the what we want
    if(((de->file_type == (unsigned char)EXT2_FT_REG_FILE) ||
      (de->file_type == (unsigned char)EXT2_FT_SYMLINK)) &&
      (strncmp(name,de->name,de->name_len)==0) &&
      (de->inode != 0)){
      unsigned int result_inode = de->inode;
      de->inode = 0;
      return result_inode;
    }
    p += de->rec_len;
    // loops through current block
    while (p < EXT2_BLOCK_SIZE) {
      struct ext2_dir_entry * cur_de = (void *) de + de->rec_len;
      if(((cur_de ->file_type == (unsigned char)EXT2_FT_REG_FILE) ||
        (cur_de ->file_type == (unsigned char)EXT2_FT_SYMLINK)) &&
        (strlen(name) == cur_de->name_len) &&
        (strncmp(name,cur_de->name,cur_de->name_len)==0) &&
        (cur_de->inode != 0)){
          // normal case
          de->rec_len = de->rec_len + cur_de->rec_len;
          return cur_de->inode;
      }
      // sets de to next dir entry and p to current total length
      p += cur_de->rec_len;
      de = (void *) de + de->rec_len;
    }
  }
  // such a legal directory (inode != 0, file_type same, file name same)
  // doesn't exist
  return 0;
}

/*
 * Restores the directory entry of a
 * file or a soft link(indicated by file_type) and 
 * returns the inode number (starts from 1)
 * of the file in dir of given inode of dir dir_inode.
 * If no files with given name can be restored
 * in current directory, returns 0.
 */
unsigned int restores_entry(char *name,
  struct ext2_inode *dir_inode){
  int j;
  // loops through all blocks of current directory
  for (j = 0; j < dir_inode->i_blocks/2; j++) {
    unsigned int curBlock = dir_inode->i_block[j];
    int p = 0;
    struct ext2_dir_entry * de = (
    struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
    // if the first entry in current block is the what we want
    if(((de->file_type == (unsigned char)EXT2_FT_REG_FILE) ||
      (de->file_type == (unsigned char)EXT2_FT_SYMLINK)) &&
      (strncmp(name,de->name,de->name_len)==0) &&
      (de->inode == 0)){
      return 0;
    }
    // loops through current block
    while (p < EXT2_BLOCK_SIZE) {
      int cur_p = entry_size(de->name_len);
      struct ext2_dir_entry * cur_de = (void *) de + entry_size(de->name_len);
      while(cur_p + dir_size_except_name + 4*sizeof(char) < de->rec_len){
        // try to avoid gaps
        // in this assignment since disk image may have
        // gaps, we need to loop bit by bit
        if(((cur_de ->file_type == (unsigned char)EXT2_FT_REG_FILE) ||
          (cur_de ->file_type == (unsigned char)EXT2_FT_SYMLINK)) &&
          (strlen(name) == cur_de->name_len) &&
          (strlen(name)==cur_de->name_len) &&
          (strncmp(name,cur_de->name,cur_de->name_len)==0)){
            cur_de -> rec_len = de -> rec_len - cur_p;
            de -> rec_len = cur_p;
            return cur_de -> inode;
        }
        cur_p += 1;
        cur_de = (void *) cur_de + 1;
      }
      // sets de to next dir entry and p to current total length
      p += de->rec_len;
      de = (void *) de + de->rec_len;
    }
  }
  // can restore such a file
  return 0;
}

/*
 * Gets the file size of a file which is in local
 * machine. path is the path to that file.
 */
unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

/*
 * Allocates blocks that are free. If the number
 * of the free blocks are less than blocks_num,
 * return False. All elements of returned unsigned
 * int array are index of block(!!!starts from 0!!!)
 */
unsigned int *allocate_blocks(int blocks_num){
  unsigned char * p = block_map;
  unsigned char buffer;
  unsigned int *blocks_allocated;
  blocks_allocated = safe_malloc(sizeof(int)*blocks_num);
  unsigned int num_allocated = 0;
  unsigned int num_scanned = 0; 
  int i;
  int j;
  for (i = 0; i < sb->s_blocks_count / 8; i++) {
    buffer = *p;
    for (j = 0; j < 8; j++) {
      if(((buffer >> j) & 0x1) == 1){
        num_scanned++;
        if(num_scanned==sb->s_blocks_count){
          return NULL;
        }
      }else{
        blocks_allocated[num_allocated]=num_scanned;
        num_allocated++;
        num_scanned++;
        if(blocks_num==num_allocated){
          return blocks_allocated;
        }
        if(num_scanned==sb->s_blocks_count){
          return NULL;
        }
      }
    }
    p++;
  }
  return NULL;
}

/*
 * Allocates available inode. If no inode available,
 * returns 0 otherwise returns the index of inode (start from 0).
 */
unsigned int allocate_inode(){
  unsigned char buffer;
  unsigned char * p = inode_map;
  unsigned int position = 0;
  int i;
  int j;
  for (i = 0; i < sb->s_blocks_count / 32; i++) {
    buffer = *p;
    for (j = 0; j < 8; j++) {
      if (((((buffer >> j) & 0x1)) == 0)&&(position>=11)){
        return position;
      }
      position++;
    }
    p++;
  }
  return 0;
}

/*
 * sets the newly created inode with given parameters
 */
int set_new_inode(unsigned int inode_index,
  unsigned short mode, unsigned int size,
  int blocks_num, unsigned int *free_blocks,int indirect) {
  int i;
  struct ext2_inode* inode = (
    struct ext2_inode *)(inodes+sizeof(struct ext2_inode)*inode_index);
  inode -> i_mode = mode;
  inode -> i_uid = 0;
  inode -> i_size = size;
  inode -> i_dtime = 0;
  inode -> i_gid = 0;
  inode -> i_links_count = 1;
  inode -> i_blocks = blocks_num*2;
  inode -> osd1 = 0;
  for (i=0;i<blocks_num;i++){
    if (i>=12){
      break;
    }
    inode->i_block[i] = free_blocks[i]+1;
  }
  if(indirect){
    inode->i_block[12] = free_blocks[blocks_num-1]+1;
  }
  inode -> i_generation = 0;
  inode -> i_file_acl = 0;
  inode -> i_dir_acl = 0;
  inode -> i_faddr = 0;
  for (i=0;i<3;i++){
    inode -> extra[i] = 0;
  }
  return 0;
}


/*
 * Sets the position of block bitmap to be num.
 * num should be 0 or 1, !!!position starts from 0!!!.
 */
int set_block_bitmap(int position, int num){
  int bit_off_in_cur_byte = position % 8;
  int byte_off = position / 8;
  unsigned char* byte = block_map + byte_off;
  *byte = ((num << bit_off_in_cur_byte) | (
    *byte & ~(1 << bit_off_in_cur_byte)));
  return 0;
}

/*
 * Sets the position of inode bitmap to be num.
 * num should be 0 or 1, !!!position starts from 0!!!.
 */
int set_inode_bitmap(int position, int num){
  int bit_off_in_cur_byte = position % 8;
  int byte_off = position / 8;
  unsigned char* byte = inode_map + byte_off;
  *byte = ((num << bit_off_in_cur_byte) | (
    *byte & ~(1 << bit_off_in_cur_byte)));
  return 0;
}



/*
 * Adds entry to the directory of inode.
 * Don't care about rec_len, this function will
 * handle it.
 */
int add_entry_to_dir(struct ext2_dir_entry *entry,
  char *name,struct ext2_inode *inode){
  int add_entry_size = entry_size(entry->name_len);
  int j;

  // loops through all blocks of current directory
  for (j = 0; j < inode->i_blocks/2; j++) {
    unsigned int curBlock = inode->i_block[j];
    int p = 0;
    struct ext2_dir_entry * de = (
    struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
    // loops through current block
    while (p < EXT2_BLOCK_SIZE) {
      if(p + de->rec_len == EXT2_BLOCK_SIZE && 
        (de->rec_len >= add_entry_size + entry_size(de->name_len))) {
        // enough space is found in blocks allocated for
        // this directory
        int prev_old_rec_len = de->rec_len;
        int prev_new_rec_len = entry_size(de->name_len);
        de->rec_len = prev_new_rec_len;
        de = (void*)de + de->rec_len;
        de->inode = entry->inode;
        de->rec_len = prev_old_rec_len - prev_new_rec_len;
        de->name_len = entry->name_len;
        de->file_type = entry->file_type;
        strncpy((void*)de + dir_size_except_name, name, entry->name_len);
        return 0;
      }
      // sets de to next dir entry and p to current total length
      p += de->rec_len;
      de = (void *) de + de->rec_len;
    }
  }
  // if all blocks are full
  unsigned int* free_blocks = allocate_blocks(1);
  if (free_blocks == NULL){
    //errno = ENOSPC;
    perror("run out of blocks for adding directory entry");
    //exit(errno);
  }
  set_block_bitmap(free_blocks[0], 1);
  struct ext2_dir_entry * new_dir = (void*
    )disk + EXT2_BLOCK_SIZE * (free_blocks[0] + 1);
  // starts to copy
  new_dir -> inode = entry->inode;
  new_dir -> rec_len = EXT2_BLOCK_SIZE;
  new_dir -> name_len = entry -> name_len;
  new_dir -> file_type = entry -> file_type;
  strncpy((void*)new_dir + dir_size_except_name, name, entry->name_len);
  // sets the parent inode(inode of directory)
  inode -> i_block[inode->i_blocks/2] = free_blocks[0] + 1;
  inode -> i_size += EXT2_BLOCK_SIZE;
  inode -> i_blocks += 2;
  // updates free blocks info
  sb->s_free_blocks_count -= 1;
  gd->bg_free_blocks_count -= 1;
  free(free_blocks);
  return 0;
}

