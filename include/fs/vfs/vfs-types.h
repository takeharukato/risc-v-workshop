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
typedef uint64_t          mnt_id;  /**< マウントポイントID           */
typedef uint64_t        vnode_id;  /**< Vnode ID                     */
typedef uint64_t        ioctx_id;  /**< I/OコンテキストID            */
typedef int32_t         fs_nlink;  /**< リンク数                     */
typedef uint32_t         fs_mode;  /**< ファイル種別/アクセスモード  */
typedef uint32_t  vfs_open_flags;  /**< open  フラグ                 */
typedef fs_mode      vnode_flags;  /**< Vnode フラグ                 */
typedef uint32_t    vfs_fd_flags;  /**< ファイルディスクリプタフラグ */
typedef uint32_t  vfs_vstat_mask;  /**< ファイル属性マスク           */
typedef uint32_t   vfs_mnt_flags;  /**< マウントフラグ               */

typedef void *    vfs_fs_private;  /**< ファイルシステム固有のスーパブロック情報          */
typedef void *   vfs_dir_private;  /**< ディレクトリ探査時のファイルシステム固有情報      */
typedef void *  vfs_file_private;  /**< ファイルディスクリプタのファイルシステム固有情報  */
typedef void *      vfs_fs_vnode;  /**< ファイルシステム固有のvnode情報 (ディスクI-node)  */


#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_TYPES_H  */
