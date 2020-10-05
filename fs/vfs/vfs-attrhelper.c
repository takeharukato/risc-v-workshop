/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  vfs file attribute helper operations                              */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>
#include <kern/page-if.h>

#include <kern/vfs-if.h>

/**
   ファイルの属性情報を初期化する
   @param[in]  stat 属性情報格納先アドレス
 */
void
vfs_init_attr_helper(vfs_file_stat *stat){

	/* ファイル属性情報を初期化する
	 */
	stat->st_vnid = VFS_INVALID_VNID;         /* v-node IDを初期化する                    */
	stat->st_dev = VFS_VSTAT_INVALID_DEVID;   /* マウント先デバイスIDを初期化する         */
	stat->st_mode = VFS_VNODE_MODE_NONE;      /* ファイル種別/アクセス属性を初期化する    */
	stat->st_nlink = 0;                       /* ファイルリンク数を初期化する             */
	stat->st_uid = VFS_VSTAT_UID_ROOT;        /* 所有者ユーザIDを初期化する               */
	stat->st_gid = VFS_VSTAT_GID_ROOT;        /* 所有者グループIDを初期化する             */
	stat->st_rdev = VFS_VSTAT_INVALID_DEVID;  /* デバイスドライバのデバイスIDを初期化する */
	stat->st_size = 0;    /* ファイルサイズを初期化する                 */
	stat->st_blksize = 0; /* ファイルシステムブロックサイズを初期化する */
	stat->st_blocks = 0;  /* 割当ブロック数を初期化する                 */
	stat->st_atime = 0;   /* 最終アクセス時刻を初期化する               */
	stat->st_mtime = 0;   /* 最終更新時刻を初期化する                   */
	stat->st_ctime = 0;   /* 最終属性更新時刻を初期化する               */

	return;
}

/**
   ファイルの属性情報をコピーする
   @param[in] dest       コピー先の属性情報のアドレス
   @param[in] src        コピー元の属性情報のアドレス
   @param[in] stat_mask  コピーする項目を表すビットマップ
 */
void
vfs_copy_attr_helper(vfs_file_stat *dest, vfs_file_stat *src, vfs_vstat_mask stat_mask) {

	if ( stat_mask & VFS_VSTAT_MASK_VNID )
		dest->st_vnid = src->st_vnid;  /* v-node IDをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_DEV )
		dest->st_dev = src->st_dev;  /* マウント先デバイスIDをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_MODE_FMT ) {

		/* ファイル種別をコピーする
		 */
		dest->st_mode &= ~VFS_VNODE_MODE_IFMT;  /* ファイル種別をクリアする */
		/* ファイル種別をコピーする */
		dest->st_mode |= src->st_mode & VFS_VNODE_MODE_IFMT;
	}

	if ( stat_mask & VFS_VSTAT_MASK_MODE_ACS ) {

		/* アクセス属性をコピーする
		 */
		dest->st_mode &= ~VFS_VNODE_MODE_ACS_MASK; /* アクセス属性をクリアする */
		/* アクセス属性をコピーする */
		dest->st_mode |= src->st_mode & VFS_VNODE_MODE_ACS_MASK;
	}

	if ( stat_mask & VFS_VSTAT_MASK_NLINK )
		dest->st_nlink = src->st_nlink;  /* リンク数をコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_UID )
		dest->st_uid = src->st_uid;  /* ユーザIDをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_GID )
		dest->st_gid = src->st_gid;  /* グループIDをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_RDEV )
		dest->st_rdev = src->st_rdev; /* デバイスドライバのデバイスIDをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_SIZE )
		dest->st_size = src->st_size;       /* ファイルサイズをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_BLKSIZE )
		dest->st_blksize = src->st_blksize; /* ブロックサイズをコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_NRBLKS )
		dest->st_blocks = src->st_blocks;   /* 割当済みブロック数をコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_ATIME )
		dest->st_atime = src->st_atime;     /* 最終アクセス時刻をコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_MTIME )
		dest->st_mtime = src->st_mtime;     /* 最終更新時刻をコピーする */

	if ( stat_mask & VFS_VSTAT_MASK_CTIME )
		dest->st_ctime = src->st_ctime;     /* 最終属性更新時刻をコピーする */
}
