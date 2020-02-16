/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Minix file operations for an image file                           */
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
create_regular_file(fs_image *handle, minix_ino dir_inum, minix_inode *dirip, 
    const char *name, uint32_t mode, minix_ino *new_inum){
	int                rc;
	minix_bitmap_idx inum;
	minix_inode new_inode;

	rc = minix_bitmap_alloc(&handle->msb, INODE_MAP, &inum);
	if ( rc != 0 )
		return -ENOSPC;  /*  ビットマップに空きがない  */

	rc = minix_add_dentry(&handle->msb, dir_inum, dirip, name, inum);
	if ( rc != 0 )
		goto free_inum_out;  /*  ディレクトリエントリ作成失敗  */

	memset(&new_inode, 0, sizeof(minix_inode));  /* I-node情報をクリア */

	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_uid, MKFS_MINIXFS_ROOT_INO_UID);
	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_gid, MKFS_MINIXFS_ROOT_INO_UID);
	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_nlinks, 1);
	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_mode, mode);
	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_size, 0);
	MINIX_D_INODE_SET(&handle->msb, &new_inode, i_mtime,
	    (uint32_t)((uint64_t)tim_get_fs_time() & 0xffffffff));

	/*
	 * I-node情報を書き出す
	 */
	rc = minix_rw_disk_inode(&handle->msb, inum, MINIX_RW_WRITING, 
	    &new_inode);
	if ( rc != 0 )
		goto free_inum_out;  /*  I-node更新失敗  */

	MINIX_D_INODE_SET(&handle->msb, dirip, i_mtime,
	    (uint32_t)((uint64_t)tim_get_fs_time() & 0xffffffff));

	rc = minix_rw_disk_inode(&handle->msb, dir_inum, MINIX_RW_WRITING, 
	    dirip); /* ディレクトリの更新時刻を更新する */
	if ( rc != 0 )
		goto free_inum_out;  /* ディレクトリの更新失敗 */

	if ( new_inum != NULL )
		*new_inum = inum;

	return 0;

free_inum_out:
	minix_bitmap_free(&handle->msb, INODE_MAP, inum); /* I-node番号を解放 */

	return rc;
}

