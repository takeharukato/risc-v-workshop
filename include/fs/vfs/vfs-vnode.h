/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system v-node                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_MOUNT_H)
#define  _FS_VFS_VFS_MOUNT_H 

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-consts.h>
#include <klib/refcount.h>
#include <kern/mutex.h>
#include <klib/rbtree.h>

/**
   V-node(仮想I-node)
 */
typedef struct _fs_mount {
	mutex                       v_mtx;  /**< マウントテーブル排他用mutex              */
	RB_ENTRY(_fs_mount)           ent;  /**< マウントテーブルのリンク                 */
	mnt_id                         id;  /**< マウントポイントID                       */
	dev_id                        dev;  /**< マウントデバイスのデバイスID             */
	srtuct _fs_container          *fs;  /**< ファイルシステム                         */
	struct _mount_table       *mnttbl;  /**< 登録先マウントテーブル                   */
	RB_HEAD(_vnode_tree, _vnode) head;  /**< vnodeテーブル                            */
	vfs_fs_private           fs_super;  /**< ファイルシステム固有のスーパブロック情報 */
	struct _vnode               *root;  /**< ルートディレクトリのvnode                */
	struct _vnode        *mount_point;  /**< マウントポイントのvnode                  */
	vfs_mnt_flags        fs_mnt_flags;  /**< マウントフラグ                           */
}fs_mount;

/**
   マウントテーブル
*/
typedef struct _mount_table{
	mutex                                mtx;  /**< マウントテーブル排他用mutex */
	RB_HEAD(_mount_tree, _fs_mount)     head;  /**< マウントテーブルエントリ    */
}mount_table;

/** 
    マウントテーブル初期化子
    @param[in] _mnttbl マウントテーブルへのポインタ
 */
#define __MNTTBL_INITIALIZER(_mnttbl) {		        \
	.mtx = __MUTEX_INITIALIZER(&((_mnttbl)->mtx)),   \
	.head  = RB_INITIALIZER(&((_mnttbl)->head)),	\
}

#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_MOUNT_H   */
