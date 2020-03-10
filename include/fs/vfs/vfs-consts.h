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

/*
 * 最大値
 */
/**  最大ファイルディスクリプタテーブルエントリ数  */
#define VFS_MAX_FD_TABLE_SIZE   ULONGLONG_C(2048)  
#define VFS_PATH_MAX            ULONGLONG_C(1024)  /**< パス長 */
#define VFS_PATH_DELIM          '/'                /**< パスデリミタ */


/**  デフォルトファイルディスクリプタテーブルエントリ数 */
#define VFS_DEFAULT_FD_TABLE_SIZE ULONGLONG_C(128)   
#define VFS_INVALID_MNTID         ULONGLONG_C(0)     /**< 無効マウントID */

#define VFS_INVALID_VNID        ULONGLONG_C(0)     /**<  無効なVNID                    */
#define VFS_DEFAULT_ROOT_VNID   ULONGLONG_C(1)     /**<  デフォルトのルートノード vnid */

#define VFS_INVALID_IOCTXID     ULONGLONG_C(0)     /**<  無効なI/OコンテキストID       */

/**
   マウント状態
 */
#define VFS_MNT_MNTFLAGS_NONE   ULONGLONG_C(0)   /**< マウントフラグデフォルト値 */
#define VFS_MNT_UNMOUNTING      ULONGLONG_C(1)   /**< アンマウント中             */
#define VFS_MNT_RDONLY          ULONGLONG_C(2)   /**< 書き込み禁止               */

/**
   openフラグ
 */
#define VFS_O_RDONLY	ULONGLONG_C(0x0)           /**<  読み取り専用でオープンする     */
#define VFS_O_WRONLY	ULONGLONG_C(0x1)           /**<  書き込み専用でオープンする     */
#define VFS_O_RDWR	ULONGLONG_C(0x2)           /**<  読み書き両用でオープンする     */
#define VFS_O_RWMASK	ULONGLONG_C(0x3)           /**<  読み書きモードを取り出すマスク */
#define VFS_O_ACCMODE	(VFS_O_RWMASK)             /**<  アクセスモードマスク           */

#define VFS_O_CREAT	ULONGLONG_C(0x0010)  /**< ファイルが存在しない場合は新規作成する  */
#define VFS_O_EXCL	ULONGLONG_C(0x0020)  /**< ファイルが存在する場合はエラーとする    */
#define VFS_O_TRUNC	ULONGLONG_C(0x0040)  /**< サイズを0に縮小する                     */
#define VFS_O_APPEND	ULONGLONG_C(0x0080)  /**< ファイルの最後に追記する                */
#define VFS_O_CLOEXEC	ULONGLONG_C(0x1000)  /**< exec時にクローズする                    */

/**
   v-nodeフラグ
 */
#define VFS_VFLAGS_SHIFT          ULONGLONG_C(16)  /**< Vnodeフラグ情報へのシフト値  */
/**  空きvnodeフラグ値  */
#define VFS_VFLAGS_FREE           ( ULONGLONG_C(0x0) << VFS_VFLAGS_SHIFT )
/**  ロード済みvnodeフラグ値  */
#define VFS_VFLAGS_VALID          ( ULONGLONG_C(0x1) << VFS_VFLAGS_SHIFT )
/**  更新済みvnodeフラグ値  */
#define VFS_VFLAGS_DIRTY          ( ULONGLONG_C(0x2) << VFS_VFLAGS_SHIFT )
/**  使用中vnodeフラグ値  */
#define VFS_VFLAGS_BUSY           ( ULONGLONG_C(0x4) << VFS_VFLAGS_SHIFT )
/**  Close On Exec vnodeフラグ値  */
#define VFS_VFLAGS_COE            ( (VFS_O_CLOEXEC) << VFS_VFLAGS_SHIFT )
/**  削除対象vnodeフラグ値  */
#define VFS_VFLAGS_DELETE         ( ULONGLONG_C(0x2000) << VFS_VFLAGS_SHIFT )

/**  空きファイルディスクリプタフラグ値  */
#define VFS_FDFLAGS_NONE         VFS_VFLAGS_FREE
/**  Close On Exec ファイルディスクリプタフラグ値  */
#define VFS_FDFLAGS_COE          VFS_VFLAGS_COE

#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */
