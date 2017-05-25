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


int main(int argc, char const **argv){
	if (argc != 3){
		fprintf(stderr,
			"Usage: ext2_rm <image file name> <absolute target file path>\n");
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

	unsigned int removed_inode_no = remove_entry(target_file_name,pdir_inode);
	// such a file or symlink is not found
	if (removed_inode_no == 0){
		errno = ENOENT;
		perror(target_file_name);
		exit(errno);
	}

	struct ext2_inode * removed_inode = (struct ext2_inode *)(
        inodes+(removed_inode_no - 1) * sizeof(struct ext2_inode));
	// decrement the link count
	removed_inode->i_links_count--;
	if(removed_inode -> i_links_count > 0){
		return 0;
	}

	removed_inode->i_dtime = time(NULL);

	int blocks_num = (removed_inode -> i_blocks)/2;
	int i;
	if(blocks_num > 12){
		for(i = 0; i < 12; i++){
			set_block_bitmap(removed_inode->i_block[i]-1,0);
			// updates free blocks info
			sb->s_free_blocks_count += 1;
			gd->bg_free_blocks_count += 1;
		}
		int indirect_block_index = removed_inode -> i_block[12] - 1;
		unsigned int* block = (void*)disk + EXT2_BLOCK_SIZE * (
			indirect_block_index + 1);
      	for (i = 0; i < blocks_num - 13; i++) {
        	set_block_bitmap((*block) - 1, 0);
        	// updates free blocks info
			sb->s_free_blocks_count += 1;
			gd->bg_free_blocks_count += 1;
        	block++;
      	}
		set_block_bitmap(indirect_block_index, 0);
		// updates free blocks info
		sb->s_free_blocks_count += 1;
		gd->bg_free_blocks_count += 1;
	}else{
		for(i = 0; i < blocks_num; i++){
			set_block_bitmap(removed_inode->i_block[i]-1,0);
			// updates free blocks info
			sb->s_free_blocks_count += 1;
			gd->bg_free_blocks_count += 1;
		}
	}
	set_inode_bitmap(removed_inode_no-1, 0);
	// updates free inodes info
	sb->s_free_inodes_count += 1;
	gd->bg_free_inodes_count += 1;
	free(target_path_dir);
	free(target_file_name);
	free(target_path);
	free(disk_path);
	return 0;
}