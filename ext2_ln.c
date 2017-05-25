#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

int hardln(char **argv){
	// unify the paths
	int i = strlen(argv[2])-1;
	while (argv[2][i] == '/'){
		argv[2][i] = '\0';
		i--;
	}
	i = strlen(argv[3])-1;
	while (argv[3][i] == '/'){
		argv[3][i] = '\0';
		i--;
	}
	// seperate paths in order to get source name, source parent path,
	// link name, link parent path
	for (i = strlen(argv[2])-1; i>=0; i--) if (argv[2][i]=='/') break;
    char *s_ppath = safe_malloc(i+1);
    strncpy(s_ppath,argv[2],i+1);
    s_ppath[i+1] = '\0';

    char *s_name = safe_malloc(strlen(argv[2])-i-1);
    strncpy(s_name,argv[2]+i+1,strlen(argv[2])-i-1);
    s_name[strlen(argv[2])-i-1] = '\0';

    for (i = strlen(argv[3])-1; i>=0; i--) if (argv[3][i]=='/') break;
    char *l_ppath = safe_malloc(i+1);
    strncpy(l_ppath,argv[3],i+1);
    l_ppath[i+1] = '\0';

    char *l_name = safe_malloc(strlen(argv[3])-i-1);
    strncpy(l_name,argv[3]+i+1,strlen(argv[3])-i-1);
    l_name[strlen(argv[3])-i-1] = '\0';
    
    struct ext2_inode * s_pdir= dir_inode(s_ppath);
    struct ext2_inode * l_pdir= dir_inode(l_ppath);
    //error check
    if (find_file(s_name,EXT2_FT_REG_FILE,s_pdir)==0){
    	fprintf(stderr, "source file does not exist\n");
    	return ENOENT;
    }

    if (find_file(l_name,EXT2_FT_REG_FILE,l_pdir) != 0){
    	fprintf(stderr, "link already exists\n");
    	return EEXIST;
    }

    if (find_file(s_name,EXT2_FT_DIR,s_pdir)!=0){
    	fprintf(stderr, "link refers to a DIR\n");
    	return EISDIR;
    }  

    // add new entry for link
    struct ext2_dir_entry* new_entry2 = safe_malloc(sizeof(struct ext2_dir_entry));
	new_entry2 -> inode = find_file(s_name,EXT2_FT_REG_FILE,s_pdir);
	new_entry2 -> rec_len = 0;
	new_entry2 -> name_len = strlen(l_name);
	new_entry2 -> file_type = EXT2_FT_REG_FILE;
	add_entry_to_dir(new_entry2, l_name, l_pdir);
    // update link count
	(get_inode(new_entry2->inode))->i_links_count++;
	//free
	free(s_name);
    free(s_ppath);
    free(l_name);
    free(l_ppath);
    free(new_entry2);
    return 0;
}

int soln(char **argv){
	// unify paths
	int i = strlen(argv[3])-1;
	while (argv[3][i] == '/'){
		argv[3][i] = '\0';
		i--;
	}

	i = strlen(argv[4])-1;
	while (argv[4][i] == '/'){
		argv[4][i] = '\0';
		i--;
	}
    // seperate paths to get source name, source parent path
    // link name, link parent path
	for (i = strlen(argv[3])-1; i>=0; i--) if (argv[3][i]=='/') break;
    char *s_ppath = safe_malloc(i+1);
    strncpy(s_ppath,argv[3],i+1);
    s_ppath[i+1] = '\0';

    char *s_name = safe_malloc(strlen(argv[3])-i-1);
    strncpy(s_name,argv[3]+i+1,strlen(argv[3])-i-1);
    s_name[strlen(argv[3])-i-1] = '\0';

    for (i = strlen(argv[4])-1; i>=0; i--) if (argv[4][i]=='/') break;
    char *l_ppath = safe_malloc(i+1);
    strncpy(l_ppath,argv[4],i+1);
    l_ppath[i+1] = '\0';

    char *l_name = safe_malloc(strlen(argv[4])-i-1);
    strncpy(l_name,argv[4]+i+1,strlen(argv[4])-i-1);
    l_name[strlen(argv[4])-i-1] = '\0';

    struct ext2_inode * s_pdir= dir_inode(s_ppath);
    struct ext2_inode * l_pdir= dir_inode(l_ppath);
    // error check
    if (find_file(s_name,EXT2_FT_REG_FILE,s_pdir)==0 ){
    	fprintf(stderr, "source file does not exist\n");
    	return ENOENT;
    }

    if (find_file(l_name,EXT2_FT_REG_FILE,l_pdir) != 0){
    	fprintf(stderr, "link already exists\n");
    	return EEXIST;
    }

    if (find_file(s_name,EXT2_FT_DIR,s_pdir)!=0){
    	fprintf(stderr, "link refers to a DIR\n");
    	return EISDIR;
    }  
    // allocate inode for link
    int new_inode_index = allocate_inode();
    unsigned int *free_blocks = allocate_blocks(1);
    set_inode_bitmap(new_inode_index, 1);
    gd->bg_free_inodes_count -= 1;
    sb->s_free_blocks_count -= 1;
	gd->bg_free_blocks_count -= 1;
    sb->s_free_inodes_count -=1;
    set_new_inode(new_inode_index, EXT2_S_IFLNK, strlen(s_ppath)+strlen(s_name), 1, free_blocks,0);
    // add new entry for link
    struct ext2_dir_entry* new_entry2 = safe_malloc(sizeof(struct ext2_dir_entry));
	new_entry2 -> inode = new_inode_index + 1;
	new_entry2 -> rec_len = 0;
	new_entry2 -> name_len = strlen(l_name);
	new_entry2 -> file_type = EXT2_FT_SYMLINK;
	add_entry_to_dir(new_entry2, l_name, l_pdir);
    // link file's content is the path of source file
    //printf("%lu\n", strlen(argv[3]));
	memcpy(disk + EXT2_BLOCK_SIZE * (free_blocks[0]+1),argv[3] , sizeof(char)*(strlen(argv[3])));
    //free
    free(s_name);
    free(s_ppath);
    free(l_name);
    free(l_ppath);
    free(new_entry2);
	return 0;
}

int main(int argc, char **argv){
	// check argvs
	if (!((argc == 4) || (argc == 5 && strcmp(argv[2],"-s") == 0))){
		fprintf(stderr, "Usage: ext2_ln <image file name>  -s[optional] <source path> <target path>\n");
		exit(1);
	}

	if(mounting_ext2(argv[1])==1){
		fprintf(stderr,"Wrong image!\n");
		exit(1);
	}

	// add links for hard link and symbolic link
	if (argc == 4) hardln(argv);
	if (argc == 5) soln(argv);

}