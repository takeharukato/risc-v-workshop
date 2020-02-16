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
	minix_inode     root;

	rc = minix_read_super(MKFS_MINIX_FS_DEVID, &sb);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not read superblock\n");
		exit(1);
	}

	rc = minix_bitmap_alloc_at(&sb, INODE_MAP, 0);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not reserve bit for I-node zero.\n");
		exit(1);
	}

	rc = minix_bitmap_alloc_at(&sb, INODE_MAP,MKFS_MINIXFS_ROOT_INO);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not reserve bit for root I-node.\n");
		exit(1);
	}

	rc = minix_bitmap_alloc_at(&sb, ZONE_MAP, 0);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not reserve bit for zone zero.\n");
		exit(1);
	}


	rc = minix_rw_disk_inode(&sb, MKFS_MINIXFS_ROOT_INO, MINIX_RW_READING, &root);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not read root I-node\n");
		exit(1);
	}

	MINIX_D_INODE_SET(&sb, &root, i_uid, MKFS_MINIXFS_ROOT_INO_UID);
	MINIX_D_INODE_SET(&sb, &root, i_gid, MKFS_MINIXFS_ROOT_INO_UID);
	MINIX_D_INODE_SET(&sb, &root, i_nlinks, MKFS_MINIXFS_ROOT_INO_LINKS);
	MINIX_D_INODE_SET(&sb, &root, i_mode, MKFS_MINIXFS_ROOT_INO_MODE);

	rc = minix_rw_disk_inode(&sb, MKFS_MINIXFS_ROOT_INO, MINIX_RW_WRITING, &root);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not write root I-node\n");
		exit(1);
	}

	rc = minix_add_dentry(&sb, MKFS_MINIXFS_ROOT_INO, &root, ".", MKFS_MINIXFS_ROOT_INO);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not add dot dentry.\n");
		exit(1);
	}

	rc = minix_add_dentry(&sb, MKFS_MINIXFS_ROOT_INO, &root, "..", MKFS_MINIXFS_ROOT_INO);
	if ( rc != 0 ) {

		fprintf(stderr, "Can not add dot-dot dentry.\n");
		exit(1);
	}

	return 0;
}

void
fill_minixv3_superblock(fs_image *img, int nr_inodes, minixv3_super_block *v3){
	uint32_t            nr_blocks;
	uint32_t      nr_inode_blocks;

	nr_blocks = truncate_align(img->size, PAGE_SIZE)/PAGE_SIZE;
		
	memset(v3, 0, sizeof(minixv3_super_block));
	
	v3->s_blocksize = PAGE_SIZE;
	v3->s_log_zone_size = 0;
	
	v3->s_ninodes = nr_inodes;
	nr_inode_blocks = 
		roundup_align(nr_inodes, (PAGE_SIZE/sizeof(minixv3_inode))) 
		/ (PAGE_SIZE/sizeof(minixv3_inode));
	v3->s_imap_blocks = roundup_align(v3->s_ninodes, 
	    v3->s_blocksize*BITS_PER_BYTE) / (v3->s_blocksize*BITS_PER_BYTE);

	if ( ( 1 + v3->s_imap_blocks + nr_inode_blocks) >= nr_blocks ) {

		fprintf(stderr, "Too many I-nodes: nr_blocks=%lu "
		    "I-node map blocks:%lu I-node blocks: %lu reserved: 2\n",
		    (unsigned long)nr_blocks, 
		    (unsigned long)v3->s_imap_blocks, 
		    (unsigned long)nr_inode_blocks);
		exit(1);
	}

	v3->s_zmap_blocks = roundup_align(nr_blocks -
	    ( 1 + v3->s_imap_blocks + nr_inode_blocks), 
	    v3->s_blocksize*BITS_PER_BYTE)/(v3->s_blocksize*BITS_PER_BYTE);
	v3->s_firstdatazone = 2 + v3->s_imap_blocks +
		v3->s_zmap_blocks + nr_inode_blocks;
	v3->s_state = MINIX_SUPER_S_STATE_CLEAN;
	v3->s_max_size = 2147483647L;
	v3->s_magic = MINIX_V3_SUPER_MAGIC;
	v3->s_zones = nr_blocks;
	v3->s_disk_version = 0;
}

void
fill_minixv12_superblock(fs_image *img, int version, int nr_inodes,
    minixv12_super_block *v12){
	int              alloc_inodes;
	uint32_t            nr_blocks;
	uint32_t      nr_inode_blocks;

	nr_blocks = truncate_align(img->size, MINIX_V2_BLOCK_SIZE)/MINIX_V2_BLOCK_SIZE;
		
	memset(v12, 0, sizeof(minixv12_super_block));

	/*
	 * I-node総数を補正
	 */
	if ( nr_inodes > 0xffff )
		alloc_inodes = 0xffff;
	else
		alloc_inodes = nr_inodes;

	if ( version == 2 ) { /* Minix Version2 ファイル名30文字 */

		v12->s_log_zone_size = 0;
	
		v12->s_ninodes = alloc_inodes;
		nr_inode_blocks = 
			roundup_align(alloc_inodes, 
			    (MINIX_V2_BLOCK_SIZE/sizeof(minixv2_inode))) 
			/ (MINIX_V2_BLOCK_SIZE/sizeof(minixv2_inode));
		v12->s_imap_blocks = roundup_align(v12->s_ninodes, 
		    MINIX_V2_BLOCK_SIZE*BITS_PER_BYTE) 
			/ (MINIX_V2_BLOCK_SIZE*BITS_PER_BYTE);

		if ( ( 1 + v12->s_imap_blocks + nr_inode_blocks) >= nr_blocks ) {

			fprintf(stderr, "Too many I-nodes: nr_blocks=%lu "
			    "I-node map blocks:%lu I-node blocks: %lu reserved: 2\n",
			    (unsigned long)nr_blocks, 
			    (unsigned long)v12->s_imap_blocks, 
			    (unsigned long)nr_inode_blocks);
			exit(1);
		}


		v12->s_zmap_blocks = roundup_align(nr_blocks -
		    ( 1 + v12->s_imap_blocks + nr_inode_blocks), 
		    MINIX_V2_BLOCK_SIZE*BITS_PER_BYTE)
			/(MINIX_V2_BLOCK_SIZE*BITS_PER_BYTE);

		v12->s_firstdatazone = 2 + v12->s_imap_blocks +
			v12->s_zmap_blocks + nr_inode_blocks;

		v12->s_state = MINIX_SUPER_S_STATE_CLEAN;
		v12->s_max_size = 0x7fffffff;
		v12->s_magic = MINIX_V2_SUPER_MAGIC2;
		v12->s_zones = nr_blocks / ( 1<< v12->s_log_zone_size );
	} else { /* Minix Version1 ファイル名14文字 */
		
		v12->s_log_zone_size = 0;
	
		v12->s_ninodes = alloc_inodes;
		nr_inode_blocks = 
			roundup_align(alloc_inodes, 
			    (MINIX_V1_BLOCK_SIZE/sizeof(minixv1_inode))) 
			/ (MINIX_V1_BLOCK_SIZE/sizeof(minixv1_inode));
		v12->s_imap_blocks = roundup_align(v12->s_ninodes, 
		    MINIX_V1_BLOCK_SIZE*BITS_PER_BYTE) 
			/ (MINIX_V1_BLOCK_SIZE*BITS_PER_BYTE);

		if ( ( 1 + v12->s_imap_blocks + nr_inode_blocks) >= nr_blocks ) {

			fprintf(stderr, "Too many I-nodes: nr_blocks=%lu "
			    "I-node map blocks:%lu I-node blocks: %lu reserved: 2\n",
			    (unsigned long)nr_blocks, 
			    (unsigned long)v12->s_imap_blocks, 
			    (unsigned long)nr_inode_blocks);
			exit(1);
		}

		v12->s_zmap_blocks = roundup_align(nr_blocks -
		    ( 1 + v12->s_imap_blocks + nr_inode_blocks), 
		    MINIX_V1_BLOCK_SIZE*BITS_PER_BYTE)
			/(MINIX_V1_BLOCK_SIZE*BITS_PER_BYTE);

		v12->s_firstdatazone = 2 + v12->s_imap_blocks +
			v12->s_zmap_blocks + nr_inode_blocks;

		v12->s_state = MINIX_SUPER_S_STATE_CLEAN;
		v12->s_max_size = (MINIX_V1_NR_DZONES 
		    + MINIX_V1_BLOCK_SIZE/sizeof(minixv1_zone)
		    + (MINIX_V1_BLOCK_SIZE/sizeof(minixv1_zone))
		    * (MINIX_V1_BLOCK_SIZE/sizeof(minixv1_zone)) )*
			( MINIX_V1_BLOCK_SIZE << v12->s_log_zone_size );
		v12->s_magic = MINIX_V1_SUPER_MAGIC;
		v12->s_nzones = nr_blocks / ( 1 << v12->s_log_zone_size );
	}
}

int
create_superblock(fs_image *img, int version, int nr_inodes){
	off_t                    addr;
	page_cache                *pc;
	minixv3_super_block       *v3;
	minixv12_super_block     *v12;

	addr = MINIX_SUPERBLOCK_BLKNO*MINIX_OLD_BLOCK_SIZE;
	pagecache_get(MKFS_MINIX_FS_DEVID, addr, &pc);

	if ( version == 3 ) {

		v3 = (minixv3_super_block *)(pc->pc_data + addr);
		fill_minixv3_superblock(img, nr_inodes, v3);
	} else {

		v12 = (minixv12_super_block *)(pc->pc_data + addr);
		fill_minixv12_superblock(img, version, nr_inodes, v12);
	}

	pagecache_put(pc);

	create_root_dir();

	return 0;
}
