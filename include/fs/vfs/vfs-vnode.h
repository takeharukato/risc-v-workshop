/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system v-node                                        */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_VNODE_H)
#define  _FS_VFS_VFS_VNODE_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-consts.h>
#include <fs/vfs/vfs-mount.h>
#include <klib/refcount.h>
#include <kern/mutex.h>
#include <kern/wqueue.h>
#include <klib/rbtree.h>

/**
   V-node(仮想I-node)
 */
typedef struct _vnode{
	mutex                      v_mtx;  /**< v-nodeの状態更新用mutex           */
	struct _refcounter        v_refs;  /**< v-node参照カウンタ                */
	vfs_fs_vnode          v_fs_vnode;  /**< ファイルシステム固有v-node        */
	RB_ENTRY(_vnode)     v_vntbl_ent;  /**< Mountポイント内のv-nodeテーブルへのエントリ */
	struct _wque_waitqueue v_waiters;  /**< v-nodeを待ち合わせているスレッド  */
	vfs_vnode_id                v_id;  /**< v-node ID                         */
	struct _fs_mount        *v_mount;  /**< 自v-nodeを格納しているMount 情報  */
	struct _fs_mount     *v_mount_on;  /**< 他のボリュームをマウントしている場合は,
					    * マウント先ボリュームのマウント情報
					    * (マウントポイントでない場合は, NULL)
					    */
	vfs_fs_mode               v_mode;  /**< ファイル種別/アクセス フラグ      */
	vfs_vnode_flags          v_flags;  /**< v-nodeのステータスフラグ          */
}vnode;

int vfs_vnode_get(vfs_mnt_id _mntid, vfs_vnode_id _vnid, struct _vnode **_outv);
int vfs_vnode_put(vfs_mnt_id _mntid, vfs_vnode_id _vnid);
int vfs_vnode_ptr_put(struct _vnode *_v);
int vfs_vnode_remove(vfs_mnt_id _mntid, vfs_vnode_id _vnid);
int vfs_vnode_ptr_remove(struct _vnode *_v);
void vfs_mark_dirty_vnode(struct _vnode *v);
void vfs_unmark_dirty_vnode(struct _vnode *v);
bool vfs_is_dirty_vnode(struct _vnode *_v);
int vfs_vnode_lock(struct _vnode *_v);
void vfs_vnode_unlock(struct _vnode *_v);
int vfs_vnode_fsync(struct _vnode *_v);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_VNODE_H   */
