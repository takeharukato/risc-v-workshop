/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file descriptor                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_DIRENT_H)
#define  _FS_VFS_VFS_DIRENT_H 

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <klib/refcount.h>
#include <kern/mutex.h>

/**
   ファイルディスクリプタ
 */
struct _file_descriptor{
	vfs_open_flags      flags; /**< ファイルディスクリプタの状態フラグ  */
	struct _refcounter   rcnt; /**< 参照カウンタ                        */
	struct _vnode         *vn; /**< 開いたファイルに対応したvnode       */
	off_t                 pos; /**< ファイルアクセス時のポジション情報  */
	vfs_file_private *private; /**< ファイル操作時の固有情報            */
}file_descriptor;

#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_DIRENT_H  */
