/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file system table                             */
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

struct        _vnode;
struct _fs_container;
struct  _mount_table;

/**
   マウントポイント情報
 */
typedef struct _fs_mount {
	mutex                         m_mtx;  /**< マウントテーブル排他用mutex              */
	RB_ENTRY(_fs_mount)           m_ent;  /**< マウントテーブルのリンク                 */
	mnt_id                         m_id;  /**< マウントポイントID                       */
	dev_id                        m_dev;  /**< マウントデバイスのデバイスID             */
	struct _fs_container          *m_fs;  /**< ファイルシステム                         */
	struct _mount_table       *m_mnttbl;  /**< 登録先マウントテーブル                   */
	RB_HEAD(_vnode_tree, _vnode) m_head;  /**< vnodeテーブル                            */
	vfs_fs_private           m_fs_super;  /**< ファイルシステム固有のスーパブロック情報 */
	struct _vnode               *m_root;  /**< ルートディレクトリのvnode                */
	struct _vnode        *m_mount_point;  /**< マウントポイントのvnode                  */
	char                  *m_mount_path;  /**< マウントパス文字列                       */
	vfs_mnt_flags         m_mount_flags;  /**< マウントフラグ                           */
}fs_mount;

/**
   マウントテーブル
*/
typedef struct _mount_table{
	mutex                                mt_mtx;  /**< マウントテーブル排他用mutex */
	RB_HEAD(_fs_mount_tree, _fs_mount)  mt_head;  /**< マウントテーブルエントリ    */
	mnt_id                           mt_last_id;  /**< 最後に割り当てたマウントID  */
}mount_table;

/** 
    マウントテーブル初期化子
    @param[in] _mnttbl マウントテーブルへのポインタ
 */
#define __MNTTBL_INITIALIZER(_mnttbl) {			        \
	.mt_mtx = __MUTEX_INITIALIZER(&((_mnttbl)->mt_mtx)),	\
	.mt_head  = RB_INITIALIZER(&((_mnttbl)->mt_head)),	\
	.mt_last_id = VFS_INVALID_MNTID,	                \
}

#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_MOUNT_H   */
