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
#include <time.h>
#include "ext2.h"
#include "ext2_util.h"

int ENOENT_error(char *target_file_name){
	errno = ENOENT;
	perror(target_file_name);
	exit(errno);
}


int main(int argc, char const **argv){
	if (argc != 3){
		fprintf(stderr,
			"Usage: ext2_restore <image file name> <absolute target file path>\n");
		return 1;
	}
	// sets the path to the disk image
	char *disk_path = safe_malloc((strlen(argv[1])+1)*sizeof(char));
	strncpy(disk_path,argv[1],strlen(argv[1]));
	disk_path[strlen(argv[1])] = '\0';

	// sets the path to the target file in the disk image
	char *target_path = safe_malloc((strlen(argv[2])+1)*sizeof(char));
	strncpy(target_path,argv[2],strlen(argv[2]));
	target_path[strlen(argv[2])] = '\0';
	if(target_path[0]!='/'){
		errno = ENOENT;
		perror(target_path);
		exit(errno);
	}
	if(target_path[strlen(argv[2])-1] == '/'){
		errno = EISDIR;
		perror(target_path);
		exit(errno);
	}

	// initializes the ext2 and 
	// checks if the image is legal
	if(mounting_ext2(disk_path)==1){
		errno = ENOENT;
		perror(disk_path);
		exit(errno);
	}

	char* target_path_dir = safe_malloc(sizeof(char)*strlen(target_path)+sizeof(char));
	strncpy(target_path_dir,target_path,strlen(target_path));
	target_path_dir[strlen(target_path)]='\0';
	char* target_file_name = safe_malloc(sizeof(char)*(EXT2_NAME_LEN+1));

	char *pre = strrchr(target_path_dir, '/');
	strncpy(target_file_name,pre+sizeof(char),strlen(pre+sizeof(char)));
	target_file_name[strlen(pre+sizeof(char))]='\0';
	*(pre+sizeof(char)) = '\0';

	struct ext2_inode * pdir_inode = dir_inode(target_path_dir);
	// if such a directory doesn'y exist in the disk
	if(pdir_inode == NULL){
		errno = ENOENT;
		perror(target_path_dir);
		exit(errno);
	}

	if((find_file(target_file_name,
			(unsigned char)EXT2_FT_REG_FILE,pdir_inode) != 0 )||
		(find_file(target_file_name,
			(unsigned char)EXT2_FT_SYMLINK,pdir_inode) != 0)){
		errno = EEXIST;
		perror(target_file_name);
		exit(errno);
	}

	unsigned int restore_inode_no = restores_entry(target_file_name,pdir_inode);
	// if such a file or symlink can not be restored
	if (restore_inode_no == 0){
		ENOENT_error(target_file_name);
	}


	struct ext2_inode * restore_inode = (struct ext2_inode *)(
        inodes+(restore_inode_no - 1) * sizeof(struct ext2_inode));
	// to see if this is a hard link or inode has been allocated
	if((restore_inode -> i_links_count > 0) ||
		(check_inode_bitmap(restore_inode_no-1)==1)){
		remove_entry(target_file_name,pdir_inode);
		ENOENT_error(target_file_name);
	}


	int blocks_num = (restore_inode -> i_blocks)/2;
	int i;
	if(blocks_num > 12){
		// checks if those blocks has not been allocated first
		// then try to reallocate them
		for(i = 0; i < 12; i++){
			if(check_block_bitmap(restore_inode->i_block[i]-1)==1){
				remove_entry(target_file_name,pdir_inode);
				ENOENT_error(target_file_name);
			}
		}
		int indirect_block_index = restore_inode -> i_block[12] - 1;
		unsigned int* block = (void*)disk + EXT2_BLOCK_SIZE * (
			indirect_block_index + 1);
      	for (i = 0; i < blocks_num - 13; i++) {
        	if(check_block_bitmap((*block) - 1)==1){
				remove_entry(target_file_name,pdir_inode);
				ENOENT_error(target_file_name);
			}
        	block++;
      	}
		if(check_block_bitmap(indirect_block_index)==1){
				remove_entry(target_file_name,pdir_inode);
				ENOENT_error(target_file_name);
		}

		// checks done, now tries to recover
		for(i = 0; i < 12; i++){
			set_block_bitmap(restore_inode->i_block[i]-1,1);
			// updates free blocks info
			sb->s_free_blocks_count -= 1;
			gd->bg_free_blocks_count -= 1;
		}

		block = (void*)disk + EXT2_BLOCK_SIZE * (
			indirect_block_index + 1);
		
      	for (i = 0; i < blocks_num - 13; i++) {
        	set_block_bitmap((*block) - 1, 1);
        	// updates free blocks info
			sb->s_free_blocks_count -= 1;
			gd->bg_free_blocks_count -= 1;
        	block++;
      	}
		set_block_bitmap(indirect_block_index, 1);
		// updates free blocks info
		sb->s_free_blocks_count -= 1;
		gd->bg_free_blocks_count -= 1;
	}else{
		for(i = 0; i < blocks_num; i++){
			if(check_block_bitmap(restore_inode->i_block[i]-1)==1){
				remove_entry(target_file_name,pdir_inode);
				ENOENT_error(target_file_name);
			}
		}

		// checks done, now tries to recover
		for(i = 0; i < blocks_num; i++){
			set_block_bitmap(restore_inode->i_block[i]-1,1);
			// updates free blocks info
			sb->s_free_blocks_count -= 1;
			gd->bg_free_blocks_count -= 1;
		}
	}

	// i_links_count should be set to 1
	restore_inode->i_links_count++;
	restore_inode->i_dtime = 0;

	set_inode_bitmap(restore_inode_no-1, 1);
	// updates free inodes info
	sb->s_free_inodes_count -= 1;
	gd->bg_free_inodes_count -= 1;
	free(target_path_dir);
	free(target_file_name);
	free(target_path);
	free(disk_path);
	return 0;
}
