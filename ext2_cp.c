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

int main(int argc, char const **argv){
	if (argc != 4){
		fprintf(stderr,
			"Usage: ext2_cp <image file name> <source path> <target path>\n");
		return 1;
	}
	char *disk_path = safe_malloc((strlen(argv[1])+1)*sizeof(char));
	strncpy(disk_path,argv[1],strlen(argv[1]));
	disk_path[strlen(argv[1])] = '\0';
	// printf("%s\n",disk_path);

	char *source_path = safe_malloc((strlen(argv[2])+1)*sizeof(char));
	strncpy(source_path,argv[2],strlen(argv[2]));
	source_path[strlen(argv[2])] = '\0';
	// printf("%s\n",source_path);

	char *target_path = safe_malloc((strlen(argv[3])+1)*sizeof(char));
	strncpy(target_path,argv[3],strlen(argv[3]));
	target_path[strlen(argv[3])] = '\0';
	// printf("%s\n",target_path);
	int target_path_len=strlen(target_path);
	// open the file that needed to be copied
	// and checks if the path to it is legal
	FILE *source_file = fopen(source_path,"r");
	if (!source_file){
		errno = ENOENT;
		perror(source_path);
		exit(errno);
	}

	if(target_path[0]!='/'){
		errno = ENOENT;
		perror(target_path);
		exit(ENOENT);
	}

	// initializes the ext2 and 
	// checks if the image is legal
	if(mounting_ext2(disk_path)==1){
		errno = ENOENT;
		perror(disk_path);
		exit(errno);
	}

	char* target_path_dir;
	char* target_file_name = safe_malloc(sizeof(char)*(EXT2_NAME_LEN+1));
	if(target_path[target_path_len-1]=='/'){
		target_path_dir=safe_malloc((target_path_len+1)*sizeof(char));
		strncpy(target_path_dir,target_path,target_path_len);
		target_path_dir[target_path_len]='\0';
		// checks if path is valid
		if(dir_inode(target_path_dir)==NULL){
			errno = ENOENT;
			perror(target_path_dir);
			exit(errno);
		}
		char *p = strrchr(source_path, '/');
		if (p==NULL){
			p=source_path-sizeof(char);
		}
		strncpy(target_file_name,p+sizeof(char),strlen(p+sizeof(char)));
		fflush(stdout);
		target_file_name[strlen(p+sizeof(char))]='\0';
	}else{
		target_path_dir = safe_malloc((target_path_len+2)*sizeof(char));
		strncpy(target_path_dir,target_path,target_path_len);
		target_path_dir[target_path_len]='\0';
		char *pre = strrchr(target_path_dir, '/');
		target_path_dir[target_path_len]='/';
		target_path_dir[target_path_len+1]='\0';
		if (dir_inode(target_path_dir)!=NULL){
			char *p = strrchr(source_path, '/');
			if (p==NULL){
				p=source_path-sizeof(char);
			}
			strncpy(target_file_name,p+sizeof(char),strlen(p+sizeof(char)));
			target_file_name[strlen(p+sizeof(char))]='\0';
		}else{
			target_path_dir[target_path_len]='\0';
			strncpy(target_file_name,pre+sizeof(char),strlen(pre+sizeof(char)));
			target_file_name[strlen(pre+sizeof(char))]='\0';
			*(pre+sizeof(char)) = '\0';
			// checks if path is valid
			if(dir_inode(target_path_dir)==NULL){
				errno = ENOENT;
				perror(target_path_dir);
				exit(errno);
			}
		}
	}

	struct ext2_inode *pdir_inode = dir_inode(target_path_dir);
	if((find_file(target_file_name,(unsigned char)EXT2_FT_REG_FILE,pdir_inode)!= 0) ||
		(find_file(target_file_name,(unsigned char)EXT2_FT_SYMLINK,pdir_inode)!= 0)){
		//file or link already exists
		errno = EEXIST;
        perror(target_file_name);
        exit(errno);
	}

	// determines how many blocks needed for data
	unsigned long source_size = get_file_size(source_path);
	int blocks_num = (source_size - 1) / EXT2_BLOCK_SIZE + 1;

	bool indirect = false;
	// determines if indirect is needed
	if (blocks_num>12){
		// needs one more block for indirect
		blocks_num += 1;
		indirect = true;
	}
	// gets a list of free blocks that can be used to copy
	unsigned int *free_blocks = allocate_blocks(blocks_num);
	if (free_blocks == NULL){
		errno = ENOSPC;
		perror("Not enough space");
    	exit(errno);
	}

	// reads the file into memory
	unsigned char* source_file_pointer = mmap(NULL,
		source_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE, fileno(source_file), 0);
	// here we start to copy files into free_blocks
	int blocks_copy_num = indirect ? blocks_num-1 : blocks_num;
	int i;
	for(i=0;i<blocks_copy_num-1;i++){
		// sets bit map
		set_block_bitmap(free_blocks[i], 1);
		memcpy(disk + EXT2_BLOCK_SIZE * (free_blocks[i] + 1),
			source_file_pointer, EXT2_BLOCK_SIZE);
		source_file_pointer += EXT2_BLOCK_SIZE;
	}
	// sets bit map
	set_block_bitmap(free_blocks[blocks_copy_num-1], 1);
	memcpy(disk+EXT2_BLOCK_SIZE*(free_blocks[blocks_copy_num-1]+1),
		source_file_pointer,source_size - EXT2_BLOCK_SIZE*(blocks_copy_num-1));
	// starts to deal with single direct
	if(indirect){
		// sets bitmap
		set_block_bitmap(free_blocks[blocks_num-1], 1);
		unsigned int* direct_block = (void*)disk +
		EXT2_BLOCK_SIZE*(free_blocks[blocks_num-1]+1);
		for(i=12;i<blocks_num-1;i++){
			*direct_block=free_blocks[i]+1;
			direct_block++;
		}
	}
	// updates free blocks info
	sb->s_free_blocks_count -= blocks_num;
	gd->bg_free_blocks_count -= blocks_num;

	// deals with inodes
	// allocates an inode for the new file
	int new_inode_index = allocate_inode();
	if (new_inode_index == 0){
		errno = ENOSPC;
		perror("too many files in system");
		exit(errno);
	}
	// sets inode bitmap
	set_inode_bitmap(new_inode_index, 1);
	// sets the inode allocated
	set_new_inode(new_inode_index, EXT2_S_IFREG,
		source_size, blocks_num, free_blocks,indirect);
	// updates free inodes info
	sb->s_free_inodes_count -= 1;
	gd->bg_free_inodes_count -= 1;

	// deals with directories
	struct ext2_dir_entry* new_entry = safe_malloc(sizeof(struct ext2_dir_entry));
	new_entry -> inode = new_inode_index + 1;
	new_entry -> rec_len = 0;
	new_entry -> name_len = strlen(target_file_name);
	new_entry -> file_type = EXT2_FT_REG_FILE;
	add_entry_to_dir(new_entry, target_file_name, pdir_inode);
	// free allocated memory
	free(new_entry);
	free(free_blocks);
	free(target_file_name);
	free(target_path_dir);
	free(target_path);
	free(source_path);
	free(disk_path);
	return 0;
}
