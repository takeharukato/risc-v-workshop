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
#include <kern/timer-if.h>

#include <kern/vfs-if.h>

/**
   ファイルの属性情報を初期化する
   @param[in]  stat 属性情報格納先アドレス
 */
void
vfs_init_attr_helper(vfs_file_stat *stat){

	/* ファイル属性情報を初期化する
	 */
	stat->st_vnid = VFS_INVALID_VNID;       /* v-node IDを初期化する                 */
	stat->st_dev = VFS_VSTAT_INVALID_DEVID; /* マウント先デバイスIDを初期化する      */
	stat->st_mode = VFS_VNODE_MODE_NONE;    /* ファイル種別/アクセス属性を初期化する */
	stat->st_nlink = 0;                     /* ファイルリンク数を初期化する      */
	stat->st_uid = VFS_VSTAT_UID_ROOT;      /* 所有者ユーザIDを初期化する        */
	stat->st_gid = VFS_VSTAT_GID_ROOT;      /* 所有者グループIDを初期化する      */
	stat->st_rdev = VFS_VSTAT_INVALID_DEVID; /* ドライバのデバイスIDを初期化する */
	stat->st_size = 0;    /* ファイルサイズを初期化する                 */
	stat->st_blksize = 0; /* ファイルシステムブロックサイズを初期化する */
	stat->st_blocks = 0;  /* 割当ブロック数を初期化する                 */

	stat->st_atime.tv_sec = 0;    /* 最終アクセス時刻を初期化する(秒)     */
	stat->st_atime.tv_nsec = 0;   /* 最終アクセス時刻を初期化する(ナノ秒) */

	stat->st_mtime.tv_sec = 0;    /* 最終更新時刻を初期化する(秒)     */
	stat->st_mtime.tv_nsec = 0;   /* 最終更新時刻を初期化する(ナノ秒) */

	stat->st_ctime.tv_sec = 0;    /* 最終属性更新時刻を初期化する(秒)     */
	stat->st_ctime.tv_nsec = 0;   /* 最終属性更新時刻を初期化する(ナノ秒) */

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

	if ( stat_mask & VFS_VSTAT_MASK_ATIME ) {

		dest->st_atime = src->st_atime;  /* 最終アクセス時刻をコピーする (秒)     */
		dest->st_atime = src->st_atime;  /* 最終アクセス時刻をコピーする (ナノ秒) */
	}

	if ( stat_mask & VFS_VSTAT_MASK_MTIME ) {

		dest->st_mtime = src->st_mtime;  /* 最終更新時刻をコピーする (秒)     */
		dest->st_mtime = src->st_mtime;  /* 最終更新時刻をコピーする (ナノ秒) */
	}

	if ( stat_mask & VFS_VSTAT_MASK_CTIME ){

		dest->st_ctime = src->st_ctime;  /* 最終属性更新時刻をコピーする (秒)     */
		dest->st_ctime = src->st_ctime;  /* 最終属性更新時刻をコピーする (ナノ秒) */
	}

}

/**
   ファイルのアクセス時刻, 更新時刻, 属性更新時刻を設定する
   @param[in]  v          操作対象ファイルのv-node情報
   @param[in]  stat       設定する時刻情報を格納した属性情報,
   NULLの場合は, 現在時刻を設定する
   @param[in]  stat_mask  更新対象時刻を表すマスク
   @retval     0          正常終了
   @note v-nodeへの参照を呼び出し元でも獲得してから呼び出す
 */
int
vfs_time_attr_helper(vnode *v, vfs_file_stat *stat, vfs_vstat_mask stat_mask){
	int                   rc;
	vfs_vstat_mask time_mask;
	vfs_file_stat         st;
	ktimespec             ts;

	time_mask = VFS_VSTAT_MASK_TIMES & stat_mask;  /*  時刻情報を取り出す */

	if ( time_mask == VFS_VSTAT_MASK_NONE )
		return 0;  /* 操作対象時刻がない */

	vfs_init_attr_helper(&st);  /* ファイル属性情報を初期化する */

	if ( stat != NULL )
		vfs_copy_attr_helper(&st, stat, time_mask);  /* 属性情報をコピーする */
	else {

		tim_walltime_get(&ts);  /* 現在時刻を取得 */

		if ( time_mask & VFS_VSTAT_MASK_ATIME ) {

			/* 最終アクセス時刻(秒)を設定する */
			st.st_atime.tv_sec = ts.tv_sec;
			/* 最終アクセス時刻(ナノ秒)を設定する */
			st.st_atime.tv_nsec = ts.tv_nsec;
		}

		if ( time_mask & VFS_VSTAT_MASK_MTIME ) {

			/* 最終更新時刻(秒)を設定する */
			st.st_mtime.tv_sec = ts.tv_sec;
			/* 最終更新時刻(ナノ秒)を設定する */
			st.st_mtime.tv_nsec = ts.tv_nsec;
		}

		if ( time_mask & VFS_VSTAT_MASK_CTIME ) {

			/* 最終属性更新時刻(秒)を設定する */
			st.st_ctime.tv_sec = ts.tv_sec;
			/* 最終属性更新時刻(ナノ秒)を設定する */
			st.st_ctime.tv_nsec = ts.tv_nsec;
		}
	}

	rc = vfs_setattr(v, &st, time_mask);  /* 属性情報を更新する  */
	if ( rc != 0 )
		goto error_out;  /* 属性情報更新に失敗した */

	return 0;

error_out:
	return rc;
}
