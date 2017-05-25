#include "ext2.h"
#include <stdlib.h>

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

// /*
//  * a dir entry size except name field
//  */
// int dir_size_except_name = sizeof(unsigned int)+sizeof(
//   unsigned short)+sizeof(unsigned char)*2;

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
int mounting_ext2(char *disk_path);

unsigned char get_type(struct ext2_inode* c_inode);
struct ext2_inode* get_inode(unsigned int num);
int check_bitmap(int num, int type);

struct ext2_inode *dir_inode(char *path);
void* safe_malloc(int size);
/*
 * Returns the inode index (starts from 1)
 * of the file in dir of given inode of dir dir_inode.
 * If no files with given name in current directory, returns 0.
 */
unsigned int find_file(char *name,unsigned char file_type,struct ext2_inode *dir_inode);
/*
 * Removes the directory entry of a
 * file or alink(indicated by file_type) and 
 * returns the inode number (starts from 1)
 * of the file in dir of given inode of dir dir_inode.
 * If no files with given name in current directory, returns 0.
 */
unsigned int remove_entry(char *name,struct ext2_inode *dir_inode);

unsigned int restores_entry(char *name,struct ext2_inode *dir_inode);


unsigned long get_file_size(const char *path);

unsigned int *allocate_blocks(int blocks_num);

unsigned int allocate_inode();

int set_new_inode(unsigned int inode_index,
  unsigned short mode, unsigned int size,
  int blocks_num, unsigned int *free_blocks,int indirect);


int set_block_bitmap(int position, int num);


int set_inode_bitmap(int position, int num);


int add_entry_to_dir(struct ext2_dir_entry *entry,
  char *name,struct ext2_inode *inode);