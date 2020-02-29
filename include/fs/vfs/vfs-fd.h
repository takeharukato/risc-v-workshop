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

/**
   ファイルディスクリプタ
 */
typedef struct _file_descriptor{
	vfs_open_flags      flags; /**< ファイルディスクリプタの状態フラグ  */
	struct _refcounter   rcnt; /**< 参照カウンタ                        */
	struct _vnode         *vn; /**< 開いたファイルに対応したvnode       */
	off_t                 pos; /**< ファイルアクセス時のポジション情報  */
	vfs_file_private *private; /**< ファイル操作時の固有情報            */
}file_descriptor;

/**
   I/Oコンテキスト
 */
typedef struct _vfs_ioctx {
	struct _mutex                              ioc_mtx;  /**< 排他用mutex          */
	struct _refcounter                        ioc_refs;  /**< 参照カウンタ         */
	/** I/Oコンテキストテーブルのリンク  */
	//RB_ENTRY(_vfs_ioctx)                        ioc_ent;
	int                                       ioc_errno;  /**< エラー番号           */
	struct _vfs_vnode                         *ioc_root;  /**< ルートディレクトリ   */
	struct _vfs_vnode                          *ioc_cwd;  /**< カレントディレクトリ */
	BITMAP_TYPE(, uint64_t, MAX_FD_TABLE_SIZE) ioc_bmap;  /**< 割当てIDビットマップ */
	/** テーブルエントリ数(単位:個)           */
	size_t                               ioc_table_size;  
	/** プロセスのファイルディスクリプタ配列  */
	struct _file_descriptor                   **ioc_fds;
}vfs_ioctx;

void vfs_filedescriptor_init(void);
void vfs_init_ioctx(void);
#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_FD_H  */
