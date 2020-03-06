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
	struct _refcounter           m_refs;  /**< マウントポイント参照カウンタ             */
	RB_ENTRY(_fs_mount)           m_ent;  /**< マウントテーブルのリンク                 */
	vfs_mnt_id                     m_id;  /**< マウントポイントID                       */
	dev_id                        m_dev;  /**< マウントデバイスのデバイスID             */
	struct _fs_container          *m_fs;  /**< ファイルシステム                         */
	struct _mount_table       *m_mnttbl;  /**< 登録先マウントテーブル                   */
	RB_HEAD(_vnode_tree, _vnode) m_head;  /**< vnodeテーブル                            */
	vfs_fs_super             m_fs_super;  /**< ファイルシステム固有のスーパブロック情報 */
	struct _vnode               *m_root;  /**< ルートディレクトリのvnode                */
	struct _vnode        *m_mount_point;  /**< マウントポイントのvnode                  */
	char                  *m_mount_path;  /**< マウントパス文字列                       */
	vfs_mnt_flags         m_mount_flags;  /**< マウントフラグ                           */
}fs_mount;

/**
   マウントテーブル
*/
typedef struct _mount_table{
	mutex                                mt_mtx;  /**< マウントテーブル排他用mutex   */
	struct _vnode                      *mt_root;  /**< システムルートディレクトリ    */
	RB_HEAD(_fs_mount_tree, _fs_mount)  mt_head;  /**< マウントテーブルエントリ      */
	vfs_mnt_id                    mt_root_mntid;  /**< ルートディレクトリマウントID  */
	vfs_mnt_id                       mt_last_id;  /**< 最後に割り当てたマウントID    */
}mount_table;

/** 
    マウントテーブル初期化子
    @param[in] _mnttbl マウントテーブルへのポインタ
 */
#define __MNTTBL_INITIALIZER(_mnttbl) {			        \
	.mt_mtx = __MUTEX_INITIALIZER(&((_mnttbl)->mt_mtx)),	\
	.mt_root = NULL, 	                                \
	.mt_root_mntid = VFS_INVALID_MNTID,                     \
	.mt_head  = RB_INITIALIZER(&((_mnttbl)->mt_head)),	\
	.mt_last_id = VFS_INVALID_MNTID,	                \
}
bool vfs_fs_mount_ref_dec(struct _fs_mount *_mount);
bool vfs_fs_mount_ref_inc(struct _fs_mount *_mount);
int vfs_fs_mount_get(vfs_mnt_id _mntid, fs_mount **_mountp);
void vfs_fs_mount_put(fs_mount *_mount);
int vfs_mount(struct _vfs_ioctx *_ioctxp, char *_path, dev_id _dev, const char *_fs_name,
	      void *_args);
int vfs_unmount(struct _vfs_ioctx *_ioctxp, char *_path);
int vfs_unmount_rootfs(void);
void vfs_init_mount_table(void);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_MOUNT_H   */
