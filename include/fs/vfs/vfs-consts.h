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

/* TODO: ULONGLONG_Cをつける */
/*
 * 最大値
 */
#define VFS_PATH_MAX            ULONGLONG_C(1024)  /**< パス長 */
#define VFS_PATH_DELIM          '/'                /**< パスデリミタ */
#define MAX_FD_TABLE_SIZE       (2048)  /**<  最大ファイルディスクリプタテーブルエントリ数  */

/**<  デフォルトファイルディスクリプタテーブルエントリ数 */
#define DEFAULT_FD_TABLE_SIZE   (128)   
#define VFS_INVALID_MNTID       (0)     /**< 無効マウントID */

#define VFS_INVALID_VNID        (0)     /**<  無効なVNID                    */
#define VFS_DEFAULT_ROOT_VNID   (1)     /**<  デフォルトのルートノード vnid */

#define VFS_INVALID_IOCTXID     (0)     /**<  無効なI/OコンテキストID       */

/** マウント状態
 */
#define VFS_MNT_MNTFLAGS_NONE   (0)     /**< マウントフラグデフォルト値 */
#define VFS_MNT_UNMOUNTING      (1)     /**< アンマウント中 */
#define VFS_MNT_RDONLY          (2)   /**< 書き込み禁止        */

/** openフラグ
 */
#define VFS_O_RDONLY	(0x0)           /**<  読み取り専用でオープンする     */
#define VFS_O_WRONLY	(0x1)           /**<  書き込み専用でオープンする     */
#define VFS_O_RDWR	(0x2)           /**<  読み書き両用でオープンする     */
#define VFS_O_RWMASK	(0x3)           /**<  読み書きモードを取り出すマスク */
#define VFS_O_ACCMODE	(VFS_O_RWMASK)  /**<  アクセスモードマスク */

#define VFS_O_CREAT	(0x0010)	/**< ファイルが存在しない場合は新規作成する  */
#define VFS_O_EXCL	(0x0020)	/**< ファイルが存在する場合はエラーとする    */
#define VFS_O_TRUNC	(0x0040)	/**< サイズを0に縮小する                     */
#define VFS_O_APPEND	(0x0080)	/**< ファイルの最後に追記する                */
#define VFS_O_CLOEXEC	(0x1000)	/**< exec時にクローズする                    */

/** v-nodeフラグ
 */
#define VFS_VFLAGS_SHIFT          (16)  /**< Vnodeフラグ情報へのシフト値  */
/**  空きvnodeフラグ値  */
#define VFS_VFLAGS_FREE           ( (0x0) << VFS_VFLAGS_SHIFT )
/**  ロード済みvnodeフラグ値  */
#define VFS_VFLAGS_VALID          ( (0x1) << VFS_VFLAGS_SHIFT )
/**  更新済みvnodeフラグ値  */
#define VFS_VFLAGS_DIRTY          ( (0x2) << VFS_VFLAGS_SHIFT )
/**  使用中vnodeフラグ値  */
#define VFS_VFLAGS_BUSY           ( (0x4) << VFS_VFLAGS_SHIFT )
/**  Close On Exec vnodeフラグ値  */
#define VFS_VFLAGS_COE            ( (VFS_O_CLOEXEC) << VFS_VFLAGS_SHIFT )
/**  削除対象vnodeフラグ値  */
#define VFS_VFLAGS_DELETE         ( (0x2000) << VFS_VFLAGS_SHIFT )

/** ファイル種別
 */
#define VFS_VNODE_MODE_NONE     (0x0)            /**<  ファイル種別未設定    */
#define VFS_VNODE_MODE_REG      ((0x1)  << 24 )  /**<  通常ファイル          */
#define VFS_VNODE_MODE_DIR      ((0x2)  << 24 )  /**<  ディレクトリ          */
#define VFS_VNODE_MODE_BLK      ((0x4)  << 24 )  /**<  ブロックデバイス      */
#define VFS_VNODE_MODE_CHR      ((0x8)  << 24 )  /**<  キャラクタ型デバイス  */
#define VFS_VNODE_MODE_LINK     ((0x10) << 24 )  /**<  シンボリックリンク    */
#define VFS_VNODE_MODE_FIFO     ((0x11) << 24 )  /**<  名前付きPIPE          */
#define VFS_VNODE_MODE_IFMT     ((0x1f) << 24 )  /**<  ファイル種別マスク    */

#define VFS_VNODE_MODE_ACS_IRWXU    (00700)     /**< ファイル所有者用ビットマスク      */
#define VFS_VNODE_MODE_ACS_IRUSR    (00400)     /**< 所有者の読み込み許可              */
#define VFS_VNODE_MODE_ACS_IWUSR    (00200)     /**< 所有者の書き込み許可              */
#define VFS_VNODE_MODE_ACS_IXUSR    (00100)     /**< 所有者の実行許可                  */
#define VFS_VNODE_MODE_ACS_IRWXG    (00070)     /**< グループ用ビットマスク            */
#define VFS_VNODE_MODE_ACS_IRGRP    (00040)     /**< グループの読み込み許可            */
#define VFS_VNODE_MODE_ACS_IWGRP    (00020)     /**< グループの書き込み許可            */
#define VFS_VNODE_MODE_ACS_IXGRP    (00010)     /**< グループの実行許可                */
#define VFS_VNODE_MODE_ACS_IRWXO    (00007)     /**< 他人 (others) 用ビットマスク      */
#define VFS_VNODE_MODE_ACS_IROTH    (00004)     /**< 他人の読み込み許可                */
#define VFS_VNODE_MODE_ACS_IWOTH    (00002)     /**< 他人の書き込み許可                */
#define VFS_VNODE_MODE_ACS_IXOTH    (00001)     /**< 他人の実行許可                    */
#define VFS_VNODE_MODE_ACS_ISUID    (04000)     /**< set-user-ID bit                   */
#define VFS_VNODE_MODE_ACS_ISGID    (02000)     /**< set-group-ID bit                  */
#define VFS_VNODE_MODE_ACS_ISVTX    (01000)     /**< スティッキー・ビット              */
#define VFS_VNODE_MODE_ACS_ISMSK    (07000)     /**< user/group/スティッキーbit マスク */
/** アクセスビットマスク  */
#define VFS_VNODE_MODE_ACS_MASK						\
	( VFS_VNODE_MODE_ACS_IRWXU |					\
	  VFS_VNODE_MODE_ACS_IRWXG |					\
	  VFS_VNODE_MODE_ACS_IRWXO |					\
	  VFS_VNODE_MODE_ACS_ISMSK )


#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */
