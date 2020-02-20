/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system constant values                               */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_CONSTS_H)
#define  _FS_VFS_VFS_CONSTS_H 

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <klib/misc.h>

#define VFS_PATH_MAX            ULONGLONG_C(1024)  /**< パス長 */
#define VFS_PATH_DELIM          '/'                /**< パスデリミタ */
#define MAX_FD_TABLE_SIZE       (2048)  /**<  最大ファイルディスクリプタテーブルエントリ数  */

/**<  デフォルトファイルディスクリプタテーブルエントリ数 */
#define DEFAULT_FD_TABLE_SIZE   (128)   
#define VFS_INVALID_MNTID       (0)     /**< 無効マウントID */

#define VFS_INVALID_VNID        (0)     /**<  無効なVNID                    */
#define VFS_DEFAULT_ROOT_VNID   (1)     /**<  デフォルトのルートノード vnid */

#define VFS_INVALID_IOCTXID     (0)     /**<  無効なI/OコンテキストID       */

#define VFS_MNT_MNTFLAGS_NONE   (0)     /**< マウントフラグデフォルト値 */
#define VFS_MNT_UNMOUNTING      (1)     /**< アンマウント中 */

#define VFS_VFLAGS_SHIFT          (16)  /**< Vnodeフラグ情報へのシフト値  */
/**  空きvnodeフラグ値  */
#define VFS_VFLAGS_FREE           ( (0x0) << VFS_VFLAGS_SHIFT )
/**  使用中vnodeフラグ値  */
#define VFS_VFLAGS_BUSY           ( (0x1) << VFS_VFLAGS_SHIFT )
/**  Close On Exec vnodeフラグ値  */
#define VFS_VFLAGS_COE            ( (VFS_O_CLOEXEC) << VFS_VFLAGS_SHIFT )
/**  削除対象vnodeフラグ値  */
#define VFS_VFLAGS_DELETE         ( (0x2000) << VFS_VFLAGS_SHIFT )

#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */
