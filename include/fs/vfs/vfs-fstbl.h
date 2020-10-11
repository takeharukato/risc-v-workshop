/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file system table                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_FSTBL_H)
#define  _FS_VFS_VFS_FSTBL_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-consts.h>
#include <klib/refcount.h>
#include <kern/mutex.h>
#include <klib/rbtree.h>

struct _vfs_file_stat;
struct _fs_calls;

/**
   ファイルシステム情報
 */
typedef struct _fs_container {
	RB_ENTRY(_fs_container)  c_ent;  /**< ファイルシステムテーブルのリンク     */
	vfs_fstype_flags       c_flags;  /**< ファイルシステム属性フラグ           */
	struct _fs_calls      *c_calls;  /**< ファイルシステム固有のファイル操作   */
	struct _refcounter      c_refs;  /**< ファイルシステム参照カウンタ         */
	char                   *c_name;  /**< ファイルシステム名を表す文字列       */
}fs_container;

/**
   ファイルシステムテーブル
*/
typedef struct _fs_table{
	mutex                                c_mtx; /**< ファイルシステムテーブル排他     */
	RB_HEAD(_fstbl_tree, _fs_container) c_head; /**< ファイルシステムテーブルエントリ */
}fs_table;

/** ファイルシステムオペレーション
 */
typedef struct _fs_calls {
	int (*fs_mount)(vfs_mnt_id _mntid, dev_id _dev,
	    void *_args, vfs_fs_super *_fs_superp, vfs_mnt_flags *_mnt_flagsp,
	    vfs_vnode_id *_root_vnidp);
	int (*fs_unmount)(vfs_fs_super _fs_super);
	int (*fs_sync)(vfs_fs_super _fs_super);
	int (*fs_lookup)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_dir_vnode,
			 const char *_name, vfs_vnode_id *_vnidp);
	int (*fs_getvnode)(vfs_fs_super _fs_super, vfs_vnode_id _vnid, vfs_fs_mode *_fs_modep,
	    vfs_fs_vnode *_fs_vnodep);
	int (*fs_putvnode)(vfs_fs_super _fs_super, vfs_vnode_id _vnid,
	    vfs_fs_vnode _fs_vnode);
	int (*fs_removevnode)(vfs_fs_super _fs_super, vfs_vnode_id _vnid,
	    vfs_fs_vnode _fs_vnode);
	int (*fs_open)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode, vfs_open_flags _omode,
	    vfs_file_private *_file_privp);
	int (*fs_close)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
	    vfs_file_private _file_priv);
	int (*fs_release_fd)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
	    vfs_file_private _file_priv);
	int (*fs_fsync)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode);
	ssize_t (*fs_read)(vfs_fs_super _fs_super,  vfs_vnode_id _vnid,
	    vfs_fs_vnode _fs_vnode, void *_buf, off_t _pos, ssize_t _len,
	    vfs_file_private _file_priv);
	ssize_t (*fs_write)(vfs_fs_super _fs_super, vfs_vnode_id _vnid,
	    vfs_fs_vnode _fs_vnode, const void *_buf, off_t _pos, ssize_t _len,
	    vfs_file_private _file_priv);
	int (*fs_seek)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
	    off_t _pos, vfs_seek_whence _whence, vfs_file_private _file_priv,
	    off_t *_new_posp);
	int (*fs_ioctl)(vfs_fs_super _fs_super,  vfs_vnode_id _vnid, vfs_fs_vnode _fs_vnode,
	    int _op, void *_buf, size_t _len, vfs_file_private _file_priv);
	int (*fs_getdents)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_dir_vnode, void *_buf,
	    off_t _off, ssize_t _buflen, ssize_t *_rdlenp);
	int (*fs_create)(vfs_fs_super _fs_super, vfs_vnode_id _fs_dir_vnid,
	    vfs_fs_vnode _fs_dir_vnode, const char *_name,
	    struct _vfs_file_stat *_stat, vfs_vnode_id *_new_vnidp);
	int (*fs_unlink)(vfs_fs_super _fs_super, vfs_vnode_id _fs_dir_vnid,
	    vfs_fs_vnode _fs_dir_vnode, const char *_name);
	int (*fs_rename)(vfs_fs_super _fs_super, vfs_vnode_id _fs_olddir_vnid,
	    vfs_fs_vnode _fs_olddir_vnode, const char *_oldname,
	     vfs_vnode_id _fs_newdir_vnid, vfs_fs_vnode _fs_newdir_vnode,
	    const char *_newname);
	int (*fs_mkdir)(vfs_fs_super _fs_super, vfs_vnode_id _fs_dir_vnid,
	    vfs_fs_vnode _fs_dir_vnode, const char *_name, vfs_vnode_id *_new_vnidp);
	int (*fs_rmdir)(vfs_fs_super _fs_super, vfs_vnode_id _fs_dir_vnid,
	    vfs_fs_vnode _fs_dir_vnode, const char *_name);
	int (*fs_getattr)(vfs_fs_super _fs_super, vfs_fs_vnode _fs_vnode,
	    vfs_vstat_mask _stat_mask, struct _vfs_file_stat *_statp);
	int (*fs_setattr)(vfs_fs_super _fs_super, vfs_vnode_id _fs_vnid,
	    vfs_fs_vnode _fs_vnode, struct _vfs_file_stat *_stat, vfs_vstat_mask _stat_mask);
}fs_calls;

#define VFS_FSTBL_FSTYPE_NONE       UINT64_C(0)    /**< ファイルシステム属性なし */
#define VFS_FSTBL_FSTYPE_PSEUDO_FS  UINT64_C(0x1)  /**< 疑似ファイルシステム     */
/**
    ファイルシステムテーブル初期化子
    @param[in] _fstbl ファイルシステムテーブルへのポインタ
 */
#define __FSTBL_INITIALIZER(_fstbl) {		         \
	.c_mtx = __MUTEX_INITIALIZER(&((_fstbl)->c_mtx)),  \
	.c_head  = RB_INITIALIZER(&((_fstbl)->c_head)),	 \
}

/**
   fs_callsの妥当性確認マクロ
   @param[in] _calls ファイルオペレーションハンドラ
*/
#define is_valid_fs_calls(_calls)				 \
	( ( (_calls) != NULL )					 \
	  && ( (_calls)->fs_getvnode != NULL )			 \
	  && ( (_calls)->fs_putvnode != NULL )			 \
	  && ( (_calls)->fs_lookup != NULL ) )

int vfs_fs_get(const char *_fs_name, struct _fs_container **_containerp);
void vfs_fs_put(struct _fs_container *_fs);
int vfs_mount(struct _vfs_ioctx *_ioctxp, char *_path, dev_id _dev, void *_args);
int vfs_register_filesystem(const char *_name, vfs_fstype_flags _fstype, struct _fs_calls *_calls);
int vfs_unregister_filesystem(const char *_name);

#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_FSTBL_H   */
