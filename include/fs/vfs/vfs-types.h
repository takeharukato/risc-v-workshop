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
#include <klib/misc.h>

#define VFS_PATH_MAX     ULONGLONG_C(1024)  /**< パス長 */
/*
 * VFS基本型
 */
typedef uint64_t         vfs_mnt_id;  /**< マウントポイントID           */
typedef uint64_t       vfs_vnode_id;  /**< Vnode ID                     */
typedef uint64_t       vfs_ioctx_id;  /**< I/OコンテキストID            */
typedef int32_t        vfs_fs_nlink;  /**< リンク数                     */
typedef uint32_t        vfs_fs_mode;  /**< ファイル種別/アクセスモード  */
typedef uint32_t     vfs_open_flags;  /**< open  フラグ                 */
typedef vfs_fs_mode vfs_vnode_flags;  /**< Vnode フラグ                 */
typedef uint32_t       vfs_fd_flags;  /**< ファイルディスクリプタフラグ */
typedef uint32_t     vfs_vstat_mask;  /**< ファイル属性マスク           */
typedef uint32_t      vfs_mnt_flags;  /**< マウントフラグ               */

typedef void *    vfs_fs_private;  /**< ファイルシステム固有のスーパブロック情報          */
typedef void *   vfs_dir_private;  /**< ディレクトリ探査時のファイルシステム固有情報      */
typedef void *  vfs_file_private;  /**< ファイルディスクリプタのファイルシステム固有情報  */
typedef void *      vfs_fs_vnode;  /**< ファイルシステム固有のvnode情報 (ディスクI-node)  */


#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_TYPES_H  */
