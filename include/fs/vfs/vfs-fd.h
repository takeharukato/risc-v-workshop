/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file descriptor                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_FD_H)
#define  _FS_VFS_VFS_FD_H 

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <klib/refcount.h>
#include <kern/mutex.h>

struct _vnode;

/**
   ファイルディスクリプタ
 */
typedef struct _file_descriptor{
	vfs_open_flags      f_flags; /**< ファイルディスクリプタの状態フラグ  */
	struct _refcounter   f_refs; /**< 参照カウンタ                        */
	struct _vnode         *f_vn; /**< 開いたファイルに対応したvnode       */
	off_t                 f_pos; /**< ファイルアクセス時のポジション情報  */
	vfs_file_private *f_private; /**< ファイル操作時の固有情報            */
}file_descriptor;

/**
   I/Oコンテキスト
 */
typedef struct _vfs_ioctx {
	struct _mutex                                   ioc_mtx; /**< 排他用mutex          */
	int                                           ioc_errno; /**< エラー番号           */
	struct _vnode                                 *ioc_root; /**< ルートディレクトリ   */
	struct _vnode                                  *ioc_cwd; /**< カレントディレクトリ */
	BITMAP_TYPE(, uint64_t, VFS_MAX_FD_TABLE_SIZE) ioc_bmap; /**< 割当てIDビットマップ */
	/** テーブルエントリ数(単位:個)           */
	size_t                                   ioc_table_size;  
	/** プロセスのファイルディスクリプタ配列  */
	struct _file_descriptor                       **ioc_fds;
}vfs_ioctx;
int  vfs_fd_add(struct _vfs_ioctx *_ioctxp, struct _file_descriptor *_f, int *_fdp);
int vfs_fd_del(struct _vfs_ioctx *_ioctxp, int _fd);
int vfs_fd_get(struct _vfs_ioctx *_ioctxp, int _fd, struct _file_descriptor **_fp);
int vfs_fd_remove(struct _vfs_ioctx *_ioctxp, struct _file_descriptor *_fp);
int vfs_fd_alloc(struct _vfs_ioctx *_ioctxp, struct _vnode *_v, vfs_open_flags _omode,
    int *_fdp);
bool vfs_fd_ref_inc(struct _file_descriptor *_f);
bool vfs_fd_ref_dec(struct _file_descriptor *_f);
int vfs_ioctx_resize_fd_table(struct _vfs_ioctx *_ioctxp, const size_t _new_size);
int vfs_ioctx_alloc(struct _vnode *root_vnode, struct _vfs_ioctx *_parent_ioctx, 
    struct _vfs_ioctx **_ioctxpp);
void vfs_ioctx_free(struct _vfs_ioctx *_ioctxp);
void vfs_init_ioctx(void);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_FD_H  */
