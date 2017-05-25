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
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    int i,j;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    struct ext2_group_desc *re = (struct ext2_group_desc *)(disk+2048);
    printf("Block group:\n");
    printf("    block bitmap: %d\n",re->bg_block_bitmap);
    printf("    inode bitmap: %d\n",re->bg_inode_bitmap);
    printf("    inode table: %d\n",re->bg_inode_table);
    printf("    free blocks: %d\n",re->bg_free_blocks_count);
    printf("    free inodes: %d\n",re->bg_free_inodes_count);
    printf("    used_dirs: %d\n",re->bg_used_dirs_count);

    unsigned char * maps = (unsigned char *) (disk + EXT2_BLOCK_SIZE * re->bg_block_bitmap);

    unsigned char * p = maps;
    unsigned char buffer;

    printf("Block bitmap:");
    for (i = 0; i < sb->s_blocks_count / 8; i++) {
      printf(" ");
      buffer = *p;
      for (j = 0; j < 8; j++) {
        printf("%d", (buffer >> j) & 0x1);
      }
      p++;
    }
    fflush(stdout);

    printf("\nInode bitmap:");
    fflush(stdout);
    maps = (unsigned char *) (disk + EXT2_BLOCK_SIZE * re->bg_inode_bitmap);
    p = maps;
    for (i = 0; i < sb->s_blocks_count / 32; i++) {
      printf(" ");
      buffer = *p;
      for (j = 0; j < 8; j++) {
        printf("%d", (buffer >> j) & 0x1);
      }
      p++;
    }

    printf("\n");

    char * inode_info = (char *)(disk + EXT2_BLOCK_SIZE * re->bg_inode_table);
    char type;

    struct ext2_inode *inode;
    int * dirRecorder = malloc(sizeof(int) * sb->s_inodes_count);
    printf("%d\n",sb->s_inodes_count);
    for (i = EXT2_ROOT_INO - 1; i < sb->s_inodes_count; i++) {
      if (i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
        continue;
      }
      inode = (struct ext2_inode *) (inode_info + i*sizeof(struct ext2_inode));

      if (inode->i_size == 0) {
        continue;
      }else{
        if (inode->i_mode & EXT2_S_IFREG) {
          type = 'f';
        } else if (inode->i_mode & EXT2_S_IFDIR) {
          type = 'd';
          *(dirRecorder + i) = 1;
        }else{
          type='e';
        }
      }

      // print
      printf("\nInodes:\n");
      printf("[%d] type: %c size: %d links: %d blocks: %d\n", i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
      printf("[%d] Blocks: ", i + 1);

      int helper;
      if (inode->i_blocks/2<11){
        helper = inode->i_blocks/2;
      }else{
        helper = 11;
      }
      for (j = 0; j < helper; j++) {
        printf(" %d", inode->i_block[j]);
      }
    }


    printf("\n\nDirectory Blocks:\n");
    for (i = EXT2_ROOT_INO - 1; i < sb->s_inodes_count; i++) {
      if (*(dirRecorder + i) == 1) {
        inode = (struct ext2_inode *)(inode_info + (i*sizeof(struct ext2_inode)));
        for (j = 0; j < inode->i_blocks/2; j++) {
          unsigned int curBlock = inode->i_block[j];
          printf("   DIR BLOCK NUM: %d (for inode %d)\n", curBlock, i + 1);
          int p = 0;
          struct ext2_dir_entry * de = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * curBlock);
          while (p < EXT2_BLOCK_SIZE) {
            char * fname = malloc(de->name_len);
            if ((unsigned int) de->file_type == EXT2_FT_REG_FILE) {
              type = 'f';
            } else if((unsigned int) de->file_type == EXT2_FT_DIR) {
              type = 'd';
            }else{
              type = 'e';
            }
            strncpy(fname, de->name, de->name_len);
            printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", de->inode, de->rec_len, de->name_len, type, fname);
            fflush(stdout);
            p += de->rec_len;
            de = (void *) de + de->rec_len;
          }
        }
      }
    }
    printf("\n");
    struct ext2_inode *cur_inode = (struct ext2_inode *)(
      inode_info+(EXT2_ROOT_INO - 1)*sizeof(struct ext2_inode));
    printf("%d\n",(unsigned int)cur_inode->i_blocks);
    int dir_size_except_name = sizeof(unsigned int)+sizeof(unsigned short)+sizeof(unsigned char)*2;
    printf("%d\n",dir_size_except_name);
    return 0;
}
