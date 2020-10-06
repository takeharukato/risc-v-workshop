/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system type definitions                              */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_TYPES_H)
#define  _FS_VFS_VFS_TYPES_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <klib/misc.h>

/*
 * VFS基本型
 */
typedef uint64_t         vfs_mnt_id;  /**< マウントポイントID           */
typedef uint64_t       vfs_vnode_id;  /**< v-node ID                    */
typedef uint64_t       vfs_ioctx_id;  /**< I/OコンテキストID            */
typedef int32_t        vfs_fs_nlink;  /**< リンク数                     */
typedef uint32_t        vfs_fs_mode;  /**< ファイル種別/アクセスモード  */
typedef uint32_t     vfs_open_flags;  /**< open  フラグ                 */
typedef uint64_t       vfs_fd_flags;  /**< ファイルディスクリプタフラグ */
typedef uint64_t    vfs_vnode_flags;  /**< v-node フラグ                */
typedef uint32_t     vfs_vstat_mask;  /**< ファイル属性マスク           */
typedef uint32_t      vfs_mnt_flags;  /**< マウントフラグ               */
typedef uint64_t   vfs_fstype_flags;  /**< ファイルシステム属性         */
typedef int         vfs_seek_whence;  /**< lseekのシーク原点            */

typedef void *      vfs_fs_super;  /**< ファイルシステム固有のスーパブロック情報          */
typedef void *     vfs_fs_dirent;  /**< ファイルシステム固有ディレクトリエントリ情報      */
typedef void *  vfs_file_private;  /**< ファイルディスクリプタのファイルシステム固有情報  */
typedef void *      vfs_fs_vnode;  /**< ファイルシステム固有のvnode情報 (ディスクI-node)  */


#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_TYPES_H  */
