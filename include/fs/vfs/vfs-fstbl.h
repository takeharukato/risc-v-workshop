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

struct _file_stat;

/**
   ファイルシステム情報
 */
typedef struct _fs_container {
	RB_ENTRY(_fs_container)  ent;  /**< ファイルシステムテーブルのリンク    */	
	struct _fs_calls      *calls;  /**< ファイルシステム固有のファイル操作  */
	struct _refcounter ref_count;  /**< ファイルシステム参照カウンタ        */
	/** 登録されているファイルシステムテーブルへのポインタ                  */
	struct _fs_table      *fstbl;  
	char    name[VFS_FSNAME_MAX];  /**< ファイルシステム名を表す文字列      */
}fs_container;

/**
   ファイルシステムテーブル
*/
typedef struct _fs_table{
	mutex                                mtx;  /**< ファイルシステムテーブル排他用mutex */
	RB_HEAD(_fstbl_tree, _fs_container) head;  /**< ファイルシステムテーブルエントリ    */
}fs_table;

/** ファイルシステムオペレーション
 */
typedef struct _fs_calls {
	int (*fs_mount)(vfs_fs_private *fs_priv, fs_id id, const char *device, 
	    void *args, vnode_id *root_vnid);
	int (*fs_unmount)(vfs_fs_private fs_priv);
	int (*fs_sync)(vfs_fs_private fs_priv);
	int (*fs_lookup)(vfs_fs_private fs_priv, vfs_fs_vnode dir,
			 const char *name, vnode_id *id, fs_mode *modep);
	int (*fs_getvnode)(vfs_fs_private fs_priv, vnode_id id, vfs_fs_vnode *v);
	int (*fs_putvnode)(vfs_fs_private fs_priv, vfs_fs_vnode v);
	int (*fs_removevnode)(vfs_fs_private fs_priv, vfs_fs_vnode v);
	int (*fs_getdents)(vfs_fs_private fs_priv, vfs_fs_vnode dir_priv, void *_buf, 
	    off_t _off, ssize_t _buflen, ssize_t *_rdlenp);
	int (*fs_open)(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_file_private *file_priv,
	    int oflags);
	int (*fs_close)(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_file_private file_priv);
	int (*fs_release_fd)(vfs_fs_private fs_priv, vfs_fs_vnode v, 
	    vfs_file_private file_priv);
	int (*fs_fsync)(vfs_fs_private fs_priv, vfs_fs_vnode v);
	ssize_t (*fs_read)(vfs_fs_private fs_priv, vfs_fs_vnode v, 
	    vfs_file_private file_priv, void *buf, off_t pos, ssize_t len);
	ssize_t (*fs_write)(vfs_fs_private fs_priv, vfs_fs_vnode v, 
	    vfs_file_private file_priv, const void *buf, off_t pos, ssize_t len);
	int (*fs_seek)(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_file_private file_priv, 
	    off_t pos, off_t *new_posp, int whence);
	int (*fs_ioctl)(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_file_private file_priv,
	    int op, void *buf, size_t len);
	int (*fs_create)(vfs_fs_private fs_priv, vfs_fs_vnode dir, const char *name, 
	    struct _file_stat *fstat, vnode_id *new_vnid);
	int (*fs_unlink)(vfs_fs_private fs_priv, vfs_fs_vnode dir, const char *name);
	int (*fs_rename)(vfs_fs_private fs_priv, vfs_fs_vnode olddir, const char *oldname, 
	    vfs_fs_vnode newdir, const char *newname);
	int (*fs_mkdir)(vfs_fs_private fs_priv, vfs_fs_vnode base_dir, const char *name);
	int (*fs_rmdir)(vfs_fs_private fs_priv, vfs_fs_vnode base_dir, const char *name);
	int (*fs_getattr)(vfs_fs_private fs_priv, vfs_fs_vnode v, vfs_vstat_mask stat_mask, 
	    struct _file_stat *stat);
	int (*fs_setattr)(vfs_fs_private fs_priv, vfs_fs_vnode v, struct _file_stat *stat, 
	    vfs_vstat_mask stat_mask);
}fs_calls;

/** 
    ファイルシステムテーブル初期化子
    @param[in] _fstbl ファイルシステムテーブルへのポインタ
 */
#define __FSTBL_INITIALIZER(_fstbl) {		        \
	.mtx = __MUTEX_INITIALIZER(&((_fstbl)->mtx)),   \
	.head  = RB_INITIALIZER(&((procdb)->head)),	\
}

/**
   fs_callsの妥当性確認マクロ
   @param[in] _calls ファイルオペレーションハンドラ
*/
#define is_valid_fs_calls(_calls)				 \
	( ( (_calls) != NULL )					 \
	    && ( (_calls)->fs_getvnode != NULL ) 		 \
	    && ( (_calls)->fs_putvnode != NULL ) 		 \
	    && ( (_calls)->fs_lookup != NULL )			 \
	    && ( (_calls)->fs_seek != NULL ) )

int vfs_fs_get(struct _fs_table *fstbl, const char *fs_name, 
    struct _fs_container **containerp);
int vfs_fs_put(struct _fs_container *_fs);
int vfs_register_filesystem(struct _fs_table *_fstbl, const char *_name, 
    struct _fs_calls *_calls);
void vfs_init_filesystem_table(void);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_FSTBL_H   */
