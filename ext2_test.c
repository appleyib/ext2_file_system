#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.c"

int main(int argc, char const **argv){
	char *disk_path = malloc((strlen(argv[1])+1)*sizeof(char));
	strncpy(disk_path,argv[1],strlen(argv[1]));
	disk_path[strlen(argv[1])] = '\0';
	// printf("%s\n",disk_path);

	// char *source_path = malloc((strlen(argv[2])+1)*sizeof(char));
	// strncpy(source_path,argv[2],strlen(argv[2]));
	// source_path[strlen(argv[2])] = '\0';
	// // printf("%s\n",source_path);

	// char *target_path = malloc((strlen(argv[3])+1)*sizeof(char));
	// strncpy(target_path,argv[3],strlen(argv[3]));
	// target_path[strlen(argv[3])] = '\0';
	// // printf("%s\n",target_path);
	// int target_path_len=strlen(target_path);
	// open the file that needed to be copied
	// and checks if the path to it is legal
	// FILE *source_file = fopen(source_path,"r");
	// if (!source_file){
	// 	fprintf(stderr,"No such file!\n");
	// 	return (int)source_file;
	// }
	// if(target_path[0]!='/'){
	// 	errno = ENOENT;
	// 	perror("Please enter an absolute path");
	// 	exit(ENOENT);
	// }

	// initializes the ext 2 and 
	// checks if the image is legal
	if(mounting_ext2(disk_path)==1){
		fprintf(stderr,"Wrong image!\n");
		exit(1);
	}

	// char* target_path_dir;
	// char* target_file_name = malloc(sizeof(char)*(EXT2_NAME_LEN+1));
	// if(target_path[target_path_len-1]=='/'){
	// 	target_path_dir=malloc((target_path_len+1)*sizeof(char));
	// 	strncpy(target_path_dir,target_path,target_path_len);
	// 	target_path_dir[target_path_len]='\0';
	// 	// checks if path is valid
	// 	if(dir_inode(target_path_dir)==NULL){
	// 		errno = ENOENT;
	// 		perror("Wrong target path");
	// 		exit(errno);
	// 	}
	// 	char *p = strrchr(source_path, '/');
	// 	if (p==NULL){
	// 		p=source_path-sizeof(char);
	// 	}
	// 	strncpy(target_file_name,p+sizeof(char),strlen(p+sizeof(char)));
	// 	fflush(stdout);
	// 	target_file_name[strlen(p+sizeof(char))]='\0';
	// }else{
	// 	target_path_dir = malloc((target_path_len+2)*sizeof(char));
	// 	strncpy(target_path_dir,target_path,target_path_len);
	// 	target_path_dir[target_path_len]='\0';
	// 	char *pre = strrchr(target_path_dir, '/');
	// 	target_path_dir[target_path_len]='/';
	// 	target_path_dir[target_path_len+1]='\0';
	// 	if (dir_inode(target_path_dir)!=NULL){
	// 		char *p = strrchr(source_path, '/');
	// 		if (p==NULL){
	// 			p=source_path-sizeof(char);
	// 		}
	// 		strncpy(target_file_name,p+sizeof(char),strlen(p+sizeof(char)));
	// 		target_file_name[strlen(p+sizeof(char))]='\0';
	// 	}else{
	// 		target_path_dir[target_path_len]='\0';
	// 		strncpy(target_file_name,pre+sizeof(char),strlen(pre+sizeof(char)));
	// 		target_file_name[strlen(pre+sizeof(char))]='\0';
	// 		*(pre+sizeof(char)) = '\0';
	// 		// checks if path is valid
	// 		if(dir_inode(target_path_dir)==NULL){
	// 			errno = ENOENT;
	// 			perror("Wrong target path");
	// 			exit(errno);
	// 		}
	// 	}
	// }
	// printf("%s\n",target_path_dir);
	// printf("%s\n",target_file_name);

	// struct ext2_inode * pdir_inode = dir_inode("/level1/level2/");
	
	// // deals with directories
	// struct ext2_dir_entry* new_entry = malloc(sizeof(struct ext2_dir_entry));
	// new_entry -> inode = 3 + 1;
	// new_entry -> rec_len = 0;
	// new_entry -> name_len = strlen("hahaha");
	// new_entry -> file_type = EXT2_FT_REG_FILE;
	// add_entry_to_dir(new_entry, "hahaha", pdir_inode);
	// free(new_entry);
	unsigned int * pointer;
	unsigned int num = 5;
	pointer = &num;
	unsigned int * pointer_old = pointer;
	pointer++;
	printf("%lu %lu\n",sizeof(int),pointer - pointer_old);
}