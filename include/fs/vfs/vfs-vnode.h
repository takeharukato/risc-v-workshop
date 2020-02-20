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
	mutex                    v_mtx;  /**< v-nodeの参照カウンタ用mutex       */
	struct _refcounter      v_refs;  /**< v-node参照カウンタ                */
	vfs_fs_vnode        v_fs_vnode;  /**< ファイルシステム固有v-node        */
	RB_ENTRY(_vnode)   v_vntbl_ent;  /**< Mountポイント内のv-nodeテーブルへのエントリ */
	struct _wque_waitqueue waiters;  /**< Vnodeを待ち合わせているスレッド  */
	vfs_mnt_id             v_mntid;  /**< マウント ID                      */
	vfs_vnode_id              v_id;  /**< Vnode ID                         */
	struct _fs_mount      *v_mount;  /**< Mount 情報                       */
	vfs_fs_mode             v_mode;  /**< ファイル種別/アクセス フラグ     */
	vfs_vnode_flags        v_flags;  /**< v-nodeのステータスフラグ         */
}vnode;


#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_VNODE_H   */
