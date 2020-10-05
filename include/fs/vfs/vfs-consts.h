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
#define VFS_MAX_FD_TABLE_SIZE   UINT64_C(2048)
#define VFS_PATH_MAX            UINT64_C(1024)  /**< パス長 */
#define VFS_PATH_DELIM          '/'                /**< パスデリミタ */


/**  デフォルトファイルディスクリプタテーブルエントリ数 */
#define VFS_DEFAULT_FD_TABLE_SIZE UINT64_C(128)
#define VFS_INVALID_MNTID         UINT64_C(0)     /**< 無効マウントID */

#define VFS_INVALID_VNID        UINT64_C(0)     /**<  無効なVNID                    */
#define VFS_DEFAULT_ROOT_VNID   UINT64_C(1)     /**<  デフォルトのルートノード vnid */

#define VFS_INVALID_IOCTXID     UINT64_C(0)     /**<  無効なI/OコンテキストID       */

/**
   マウント状態
 */
#define VFS_MNT_MNTFLAGS_NONE   UINT64_C(0)   /**< マウントフラグデフォルト値 */
#define VFS_MNT_UNMOUNTING      UINT64_C(1)   /**< アンマウント中             */
#define VFS_MNT_RDONLY          UINT64_C(2)   /**< 書き込み禁止               */
#define VFS_MNT_NOATIME         UINT64_C(4)   /**< アクセス時刻を更新しない   */

/**
   openフラグ
 */
#define VFS_O_NONE	 UINT64_C(0x00000000)  /**< フラグなし                          */
#define VFS_O_RDONLY	 UINT64_C(0x00000001)  /**< 読み取り専用でオープンする          */
#define VFS_O_WRONLY	 UINT64_C(0x00000002)  /**<  書き込み専用でオープンする         */
#define VFS_O_RDWR	 UINT64_C(0x00000003)  /**< 読み書き両用でオープンする          */
#define VFS_O_RWMASK	 VFS_O_RDWR            /**< 読み書きモードを取り出すマスク      */
#define VFS_O_RESERVE1	 UINT64_C(0x00000004)  /**< 予約                                */
#define VFS_O_RESERVE2	 UINT64_C(0x00000008)  /**< 予約                                */
#define VFS_O_CREAT	 UINT64_C(0x00000010)  /**< ファイルが存在しない場合新規作成    */
#define VFS_O_EXCL	 UINT64_C(0x00000020)  /**< 排他オープン                        */
#define VFS_O_NOCTTY	 UINT64_C(0x00000040)  /**< 制御端末を開かない                  */
#define VFS_O_TRUNC	 UINT64_C(0x00000100)  /**< サイズを0に縮小する                 */
#define VFS_O_APPEND	 UINT64_C(0x00000200)  /**< ファイルの最後に追記する            */
#define VFS_O_NONBLOCK	 UINT64_C(0x00000400)  /**< 非閉塞オープンする                  */
#define VFS_O_SYNC	 UINT64_C(0x00001000)  /**< 同期書き込みでオープンする          */
#define VFS_O_FASYNC	 UINT64_C(0x00002000)  /**< 非同期通知フラグ                    */
#define VFS_O_DIRECT	 UINT64_C(0x00004000)  /**< ダイレクトI/O                       */
#define VFS_O_LARGEFILE	 UINT64_C(0x00010000)  /**< ラージファイルとして開く            */
#define VFS_O_DIRECTORY	 UINT64_C(0x00020000)  /**< ディレクトリをオープンする          */
#define VFS_O_NOFOLLOW   UINT64_C(0x00040000)  /**< シンボリックリンクをたどらない      */
#define VFS_O_NOATIME    UINT64_C(0x00080000)  /**< アクセス時刻を更新しない            */
#define VFS_O_CLOEXEC	 UINT64_C(0x00800000)  /**< exec時にクローズする                */
/** オープンフラグマスク(符号ビットを使用しない32bit値)   */
#define VFS_O_FLAGS_MASK UINT64_C(0xefffffff)
/** ディレクトリオープン時に設定可能なフラグ */
#define VFS_O_OPENDIR_MASK ( VFS_O_RDONLY |				\
	    ( ~(VFS_O_RWMASK|VFS_O_CREAT|VFS_O_EXCL|			\
		VFS_O_NOCTTY|VFS_O_TRUNC|VFS_O_SYNC|VFS_O_FASYNC|VFS_O_DIRECT) ) )

/**
   v-nodeフラグ (64bit fs/vfs/vfs-types.h 参照)
 */
#define VFS_VFLAGS_SHIFT          UINT64_C(16)  /**< v-nodeフラグ情報へのシフト値  */
/**  空きvnodeフラグ値  */
#define VFS_VFLAGS_FREE           ( UINT64_C(0x0) << VFS_VFLAGS_SHIFT )
/**  ロード済みvnodeフラグ値  */
#define VFS_VFLAGS_VALID          ( UINT64_C(0x1) << VFS_VFLAGS_SHIFT )
/**  更新済みvnodeフラグ値  */
#define VFS_VFLAGS_DIRTY          ( UINT64_C(0x2) << VFS_VFLAGS_SHIFT )
/**  使用中vnodeフラグ値  */
#define VFS_VFLAGS_BUSY           ( (VFS_O_RESERVE1) << VFS_VFLAGS_SHIFT )
/**  削除対象vnodeフラグ値  */
#define VFS_VFLAGS_DELETE         ( (VFS_O_RESERVE2) << VFS_VFLAGS_SHIFT )

/*
 * ファイルディスクリプタフラグ  (64bit fs/vfs/vfs-types.h 参照)
 */
/** オープンフラグからファイルディスクリプタフラグのシフト値 */
#define VFS_FDFLAGS_SHIFT         UINT64_C(0)
#define VFS_FDFLAGS_MASK          VFS_O_FLAGS_MASK  /**< オープンフラグのマスク値 */
/**
   オープンフラグからファイルディスクリプタフラグを算出する
   @param[in] _oflags オープンフラグ
   @return ファイルディスクリプタフラグ値
 */
#define VFS_OFLAGS_TO_FDFLAGS(_oflags)			\
	( ( (uint64_t)(_oflags) & VFS_FDFLAGS_MASK ) << VFS_FDFLAGS_SHIFT )

/** 読み取りオープン */
#define VFS_FDFLAGS_RDONLY         VFS_OFLAGS_TO_FDFLAGS(VFS_O_RDONLY)
/** 書き込みオープン */
#define VFS_FDFLAGS_WRONLY         VFS_OFLAGS_TO_FDFLAGS(VFS_O_WRONLY)
/**< 読み書き両用でオープン */
#define VFS_FDFLAGS_RDWR           VFS_OFLAGS_TO_FDFLAGS(VFS_O_RDWR)
/**< 読み書きモードを取り出すマスク */
#define VFS_FDFLAGS_RWMASK         VFS_OFLAGS_TO_FDFLAGS(VFS_O_RWMASK)
/**  空きファイルディスクリプタフラグ値  */
#define VFS_FDFLAGS_NONE           VFS_OFLAGS_TO_FDFLAGS(VFS_O_NONE)
/** 追記書き込みフラグ */
#define VFS_FDFLAGS_APPEND         VFS_OFLAGS_TO_FDFLAGS(VFS_O_APPEND)
/** 非閉塞オープンフラグ */
#define VFS_FDFLAGS_NONBLOCK       VFS_OFLAGS_TO_FDFLAGS(VFS_O_NONBLOCK)
/** 同期書き込みフラグ */
#define VFS_FDFLAGS_SYNC           VFS_OFLAGS_TO_FDFLAGS(VFS_O_SYNC)
/** 非同期通知フラグ */
#define VFS_FDFLAGS_FASYNC         VFS_OFLAGS_TO_FDFLAGS(VFS_O_FASYNC)
/** ダイレクトI/Oフラグ */
#define VFS_FDFLAGS_DIRECT         VFS_OFLAGS_TO_FDFLAGS(VFS_O_DIRECT)
/** ラージファイルフラグ */
#define VFS_FDFLAGS_LARGEFILE      VFS_OFLAGS_TO_FDFLAGS(VFS_O_LARGEFILE)
/** ディレクトリオープンフラグ */
#define VFS_FDFLAGS_DIRECTORY      VFS_OFLAGS_TO_FDFLAGS(VFS_O_DIRECTORY)
/** アクセス時刻更新無効フラグ */
#define VFS_FDFLAGS_NOATIME        VFS_OFLAGS_TO_FDFLAGS(VFS_O_NOATIME)
/**  Close On Exec vnodeフラグ値  */
#define VFS_FDFLAGS_CLOEXEC        VFS_OFLAGS_TO_FDFLAGS(VFS_O_CLOEXEC)

#endif  /*  !ASM_FILE */
#endif  /*  _FS_VFS_VFS_CONSTS_H  */
