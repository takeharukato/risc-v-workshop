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
struct _vfs_page_cache;
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

int vfs_fs_get(const char *_fs_name, struct _fs_container **_containerp);
void vfs_fs_put(struct _fs_container *_fs);
int vfs_mount(struct _vfs_ioctx *_ioctxp, char *_path, dev_id _dev, void *_args);
int vfs_register_filesystem(const char *_name, vfs_fstype_flags _fstype, struct _fs_calls *_calls);
int vfs_unregister_filesystem(const char *_name);

#endif  /*  !ASM_FILE  */
#endif  /* _FS_VFS_VFS_FSTBL_H   */
