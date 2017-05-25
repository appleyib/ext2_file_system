#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

int num = 0;
void check_bit_map(){
	int i,j;
	unsigned char * maps = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
    unsigned char * p = maps;
    unsigned char buffer;
    int free_block_count = 0, free_inode_count = 0;
    // count free bits in block bitmap
    for (i = 0; i < sb->s_blocks_count / 8; i++) {
      buffer = *p;
      for (j = 0; j < 8; j++) {
      	if (!((buffer >> j) & 0x1)) free_block_count++;
      }
      p++;
    }
    // check free bits in inode bitmap
    maps = (unsigned char *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
    p = maps;
    for (i = 0; i < sb->s_blocks_count / 8; i++) {
      buffer = *p;
      for (j = 0; j < 8; j++) {
      	if(!((buffer >>j) & 0x1)) free_inode_count++;
      }
      p++;
    }
    // compare free counts in sb and gd with free bits in inode, block bitmap
    if (sb -> s_free_blocks_count != free_block_count){
    	int off = abs(sb->s_free_blocks_count - free_block_count);
    	num += off;
    	sb->s_free_blocks_count = free_block_count;
    	printf("Fixed:Superblock's free blocks counter was off by %d compared to the bitmap \n",off);
    }
    if(sb->s_free_inodes_count != free_inode_count){
        sb->s_free_inodes_count = free_inode_count;
        int off = abs(free_inode_count - sb->s_free_inodes_count);
        num += off;
        printf("Fixed:Superblock's free inodes counter was off by %d compared to the bitmap\n",off);
    }
    if(gd->bg_free_blocks_count != free_block_count){
        gd->bg_free_blocks_count = free_block_count;
        int off = abs(free_block_count - gd->bg_free_blocks_count);
        num += off;
        printf("Fixed:block group's free blocks counter was off by %d compared to the bitmap\n",off);
    }
    if(gd->bg_free_inodes_count != free_inode_count){
        gd->bg_free_inodes_count = free_inode_count;
        int off = (free_inode_count - gd->bg_free_inodes_count);
        num += off;
        printf("Fixed:block group's free inodes counter was off by %d compared to the bitmap\n",off);
    }
}

void check(struct ext2_inode *cur_inode,int inode_num){
    int i,j,k;
    struct ext2_inode *child_node;
    struct ext2_dir_entry * e;
    unsigned int * indirect;
	for (j = 0;j<12;j++){
		if (cur_inode -> i_block[j] != 0){
			e = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * cur_inode->i_block[j]);
			int p = 0;
			while (p < EXT2_BLOCK_SIZE){
				// get inode of one child
				child_node = get_inode(e->inode);
				// check type
				if (e->file_type != get_type(child_node)){
					e->file_type = get_type(child_node);
					num ++;
					printf("Fixed: Entry type vs inode mismatch: inode [%d]\n",e->inode);
				}
                // check inode bitmap in-use
				if(check_bitmap(e -> inode, 1) == 1){
	    			set_inode_bitmap(e -> inode-1, 1);
	   				num++;
	   				gd->bg_free_inodes_count--;
					sb -> s_free_inodes_count--;
	    			printf("Fixed: inode [%d] not marked as in-use\n", e-> inode);
				}
                // check dtime
				if(child_node->i_dtime != 0){
					child_node->i_dtime = 0;
					num ++;
					printf("Fixed: valid inode marked for deletion : [%d]\n", e->inode);
				}
                // check block bitmap for inode self
				if (check_bitmap(cur_inode -> i_block[j],0) ==1){
					set_block_bitmap(cur_inode -> i_block[j]-1,1);
					gd->bg_free_blocks_count--;
					sb -> s_free_blocks_count--;
					num++;
					printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", cur_inode->i_block[j], inode_num);
				}
                // check block bitmap for content in one file
				if((get_type(child_node) == EXT2_FT_REG_FILE) || (get_type(child_node) == EXT2_FT_SYMLINK) || (e->inode == EXT2_GOOD_OLD_FIRST_INO)){
					for (i = 0; i<12; i++){
						if (child_node -> i_block[i]!=0){
							if (check_bitmap(child_node->i_block[i], 0)==1){
								set_block_bitmap(child_node->i_block[i]-1, 1);
	   							num++;
	   							gd->bg_free_blocks_count--;
								sb->s_free_blocks_count--;
								printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", child_node->i_block[i], e->inode);
							}
						}
					}
					// if indirect
					if (child_node -> i_block[12]!=0){
						indirect = (unsigned int *)(disk + EXT2_BLOCK_SIZE * (child_node->i_block[12]));
						for (k = 0; k < EXT2_BLOCK_SIZE/4; k++){
							if(indirect[k] != 0 && check_bitmap(indirect[k],0)==1){
								num++;
								gd->bg_free_blocks_count--;
								sb->s_free_blocks_count--;
								printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n",indirect[k], e->inode);
								set_block_bitmap(indirect[k]-1,1);

							}
						}

					}

				}
				// recursively check
				if ((strcmp(e->name,".")!=0) && (strcmp(e->name,"..")!=0) && (get_type(child_node) == EXT2_FT_DIR) && (e->inode > EXT2_GOOD_OLD_FIRST_INO)) {
                    check(child_node,e->inode);
                }
                // update entry
				p += e->rec_len;
				e = (void *) e + e->rec_len;
			}

		}
	}
	}
int main(int argc, char **argv){
	// check argvs
	if (argc!=2){
		fprintf(stderr,"Usage: ext2_checker <image name>\n");
		exit(1);
	}
	if(mounting_ext2(argv[1])==1){
		fprintf(stderr,"Wrong image!\n");
		exit(1);
	}
	// check bitmap
	check_bit_map();
	// recursively check each inode, starts with root
	check(get_inode(EXT2_ROOT_INO),EXT2_ROOT_INO);
	printf("%d file system inconsistencies repaired!\n", num);
	return 0;
}