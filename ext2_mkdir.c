#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

int main(int argc, char **argv){
    //check whether argvs are valid
	if (argc != 3){
		fprintf(stderr, "Usage: ext2_mkdir <image file name> <absolute path>\n");
		exit(1);
	}

	if(mounting_ext2(argv[1])==1){
		fprintf(stderr,"Wrong image!\n");
		exit(1);
	}
    if (strcmp(argv[2],"/")==0){
    	fprintf(stderr,"Can not make root\n");
		return ENOENT;
    }
    // allocation new inode for new dir.
	int new_inode_index = allocate_inode();
	if (new_inode_index == 0){
		errno = ENOSPC;
		perror("too many files in system");
		exit(errno);
	}

	char *d_path;

    // unify the new directory path to '/xx/xx/'
	if (argv[2][strlen(argv[2])-1]!='/'){
		d_path = safe_malloc(strlen(argv[2])+2);
		strncpy(d_path,argv[2],strlen(argv[2])+1);
		strncat(d_path,"/",1);
		d_path[strlen(argv[2])+1]='\0';
	}else{
		d_path = safe_malloc(strlen(argv[2])+1);
		strncpy(d_path,argv[2],strlen(argv[2]));
		d_path[strlen(argv[2])]='\0';
	}
    
    // check path
    if (dir_inode(d_path)!=NULL){
    	fprintf(stderr,"Directory already exits");
		return EEXIST;
    } 
    //update variables in gd and sb
    set_inode_bitmap(new_inode_index, 1);
    gd->bg_free_inodes_count -= 1;
    sb->s_free_blocks_count -= 1;
	gd->bg_free_blocks_count -= 1;
    sb->s_free_inodes_count -=1;
    gd -> bg_used_dirs_count++;
    unsigned int *free_blocks = allocate_blocks(1);
    set_block_bitmap(free_blocks[0],1);
    set_new_inode(new_inode_index, EXT2_S_IFDIR, EXT2_BLOCK_SIZE, 1, free_blocks,0);

    //seperate path in order to get name of new dir
    //inode_num and inode of parent dir
    int i = 0;
    for (i = strlen(d_path)-2; i>=0;i--){
    	if (d_path[i] == '/') break;
    }
    char * name = safe_malloc(strlen(d_path)-i-1);
    strncpy(name,d_path+i+1,strlen(d_path)-i-1);
    name[strlen(d_path)-i-2] = '\0';
    struct ext2_inode *pdir_inode;
    int pdir_inode_index = 0;
    if (i==0) {
    	pdir_inode = (struct ext2_inode *)(inodes+sizeof(struct ext2_inode)*1);
    	pdir_inode_index = EXT2_ROOT_INO;
    }else{
    	char * p = safe_malloc(i+1);
    	strncpy(p,argv[2],i+1);
    	p[i+1] = '\0';
        for (i = strlen(p)-2; i>=0;i--){
    	    if (p[i] == '/') break;
        }
    	pdir_inode = dir_inode(p); 
    	char *pname = safe_malloc(strlen(p)-i-1);
    	strncpy(pname,p+i+1,strlen(p)-i-1);
    	pname[strlen(p)-i-2] = '\0';
    	p[i+1] = '\0';
    	pdir_inode_index = find_file(pname,EXT2_FT_DIR,dir_inode(p));
        free(p);
        free(pname);
    }
    // check parent dir.
    if (!pdir_inode){
        fprintf(stderr, "parent directory does not exist\n");
        return ENOENT;
    }
    // add new entry in parent dir
    struct ext2_dir_entry* new_entry3 = safe_malloc(sizeof(struct ext2_dir_entry));
	new_entry3 -> inode = new_inode_index + 1;
	new_entry3 -> rec_len = 0;
	new_entry3 -> name_len = strlen(name);
	new_entry3 -> file_type = EXT2_FT_DIR;
	add_entry_to_dir(new_entry3, name, pdir_inode);
	pdir_inode->i_links_count++;
    // add "." entry in new dir.
	struct ext2_dir_entry * de = (
    struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * (free_blocks[0]+1));
    memcpy(de -> name, ".", strlen("."));
    de -> rec_len = 1024;
    de -> inode = new_inode_index + 1;
    de -> file_type = EXT2_FT_DIR;
    de -> name_len = strlen(".");
    // add ".." entry in new dir
    struct ext2_inode * d =dir_inode(d_path);
    struct ext2_dir_entry* new_entry2 = safe_malloc(sizeof(struct ext2_dir_entry));
	new_entry2 -> inode = pdir_inode_index;
	new_entry2 -> rec_len = 0;
	new_entry2 -> name_len = 2;
	new_entry2 -> file_type = EXT2_FT_DIR;
	add_entry_to_dir(new_entry2, "..", d);
	d->i_links_count++;

    // free
    free(d_path);
    free(new_entry3);
    free(new_entry2);
    free(name);
	return 0;
}
