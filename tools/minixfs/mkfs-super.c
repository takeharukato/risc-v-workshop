/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix super block operations for an image file                    */
/*                                                                    */
/**********************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <klib/freestanding.h>
#include <kern/kern-consts.h>
#include <klib/klib-consts.h>
#include <klib/misc.h>
#include <klib/align.h>
#include <klib/compiler.h>
#include <kern/kern-types.h>
#include <kern/page-macros.h>

#include <kern/dev-pcache.h>

#include <fs/minixfs/minixfs.h>


#include <minixfs-tools.h>
#include <utils.h>

int
create_root_dir(void){
	int rc;
	minix_super_block sb;

	rc = minix_read_super(MKFS_MINIX_FS_DEVID, &sb);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not read superblock\n");
		exit(1);
	}

	return 0;
}
int
create_superblock(fs_image *img, int version, int nr_inodes){
	off_t                    addr;
	page_cache                *pc;
	minixv3_super_block       *v3;
	uint32_t            nr_blocks;
	uint32_t      nr_inode_blocks;

	addr = MINIX_SUPERBLOCK_BLKNO*MINIX_OLD_BLOCK_SIZE;
	pagecache_get(MKFS_MINIX_FS_DEVID, addr, &pc);
	if ( version == 3 ) {

		nr_blocks = truncate_align(img->size, PAGE_SIZE)/PAGE_SIZE;
		
		v3 = (minixv3_super_block *)(pc->pc_data + addr);
		memset(v3, 0, sizeof(minixv3_super_block));

		v3->s_blocksize = PAGE_SIZE;
		v3->s_log_zone_size = 0;

		v3->s_ninodes = nr_inodes;
		nr_inode_blocks = 
			roundup_align(nr_inodes, (PAGE_SIZE/sizeof(minixv3_inode))) 
			/ (PAGE_SIZE/sizeof(minixv3_inode));
		v3->s_imap_blocks = roundup_align(v3->s_ninodes, 
		    v3->s_blocksize*BITS_PER_BYTE) / (v3->s_blocksize*BITS_PER_BYTE);
		v3->s_zmap_blocks = roundup_align(nr_blocks -
		    ( 1 + v3->s_imap_blocks + nr_inode_blocks), 
		    v3->s_blocksize*BITS_PER_BYTE)/(v3->s_blocksize*BITS_PER_BYTE);
		v3->s_firstdatazone = 2 + v3->s_imap_blocks +
			v3->s_zmap_blocks + nr_inode_blocks;
		v3->s_max_size = 2147483647L;
		v3->s_magic = MINIX_V3_SUPER_MAGIC;
		v3->s_zones = nr_blocks;
		v3->s_disk_version = 0;
	}

	create_root_dir();

	pagecache_put(pc);

	return 0;
}
