/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system file attribute                                */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_FD_H)
#define  _FS_VFS_VFS_ATTR_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <fs/vfs/vfs-types.h>
#include <klib/refcount.h>
#include <kern/mutex.h>
#include <kern/timer-if.h>

struct _vnode;

/**
   ファイル属性
 */
typedef struct _vfs_file_stat {
	vfs_vnode_id          st_vnid; /**< vnode id                                */
	dev_id                 st_dev; /**< ファイルがあるデバイスの ID             */
	vfs_fs_mode           st_mode; /**< ファイル種別/アクセス権                 */
	vfs_fs_nlink         st_nlink; /**< リンク数                                */
	cred_id                st_uid; /**< ユーザID                                */
	cred_id                st_gid; /**< グループID                              */
	dev_id                st_rdev; /**< デバイスドライバのデバイスID            */
	off_t                 st_size; /**< ファイルサイズ                          */
	off_t              st_blksize; /**< ファイルシステム I/O でのブロックサイズ */
	obj_cnt_type        st_blocks; /**< 割り当てられたブロック数                */
	struct _ktimespec    st_atime; /**< 最終アクセス時刻                        */
	struct _ktimespec    st_mtime; /**< 最終修正時刻                            */
	struct _ktimespec    st_ctime; /**< 最終状態変更時刻                        */
}vfs_file_stat;
/*
 * 操作対象メンバ
 */
#define VFS_VSTAT_MASK_NONE      (0x0000)  /**< 属性情報を操作しない        */
#define VFS_VSTAT_MASK_VNID      (0x0001)  /**< Vnode ID                    */
#define VFS_VSTAT_MASK_DEV       (0x0002)  /**< ファイルがあるデバイスのID  */
#define VFS_VSTAT_MASK_MODE_FMT  (0x0004)  /**< ファイル種別                */
#define VFS_VSTAT_MASK_MODE_ACS  (0x0008)  /**< アクセス権                  */
#define VFS_VSTAT_MASK_NLINK     (0x0010)  /**< リンク数                    */
#define VFS_VSTAT_MASK_UID       (0x0020)  /**< ユーザID                    */
#define VFS_VSTAT_MASK_GID       (0x0040)  /**< グループID                  */
#define VFS_VSTAT_MASK_RDEV      (0x0080)  /**< ドライバのデバイスID        */
#define VFS_VSTAT_MASK_SIZE      (0x0100)  /**< ファイルサイズ              */
#define VFS_VSTAT_MASK_BLKSIZE   (0x0200)  /**< ブロックサイズ              */
#define VFS_VSTAT_MASK_NRBLKS    (0x0400)  /**< ブロック数                  */
#define VFS_VSTAT_MASK_ATIME     (0x0800)  /**< 最終アクセス時刻            */
#define VFS_VSTAT_MASK_MTIME     (0x1000)  /**< 最終更新時刻                */
#define VFS_VSTAT_MASK_CTIME     (0x2000)  /**< 最終状態更新時刻            */

#define VFS_VSTAT_UID_ROOT      (0)       /**< rootユーザID                 */
#define VFS_VSTAT_GID_ROOT      (0)       /**< rootグループID               */
#define VFS_VSTAT_INVALID_DEVID (0)       /**< 無効デバイスID               */

/** ファイル作成時の状態コピーマスク  */
#define VFS_VSTAT_MASK_CREATE \
	( VFS_VSTAT_MASK_MODE_FMT | VFS_VSTAT_MASK_MODE_ACS | \
	    VFS_VSTAT_MASK_UID | VFS_VSTAT_MASK_GID )

 /** ファイル時刻更新マスク  */
#define VFS_VSTAT_MASK_TIMES \
	( VFS_VSTAT_MASK_ATIME | VFS_VSTAT_MASK_MTIME | VFS_VSTAT_MASK_CTIME )

 /** ファイル属性更新条件マスク  */
#define VFS_VSTAT_MASK_CHATTR \
	( VFS_VSTAT_MASK_MODE_ACS |					\
	  VFS_VSTAT_MASK_UID      |					\
	  VFS_VSTAT_MASK_GID )

 /** 属性情報反映対象マスク  */
#define VFS_VSTAT_MASK_SETATTR						\
	( ( VFS_VSTAT_MASK_CHATTR   |					\
	    VFS_VSTAT_MASK_SIZE     |					\
	    VFS_VSTAT_MASK_TIMES ) )

 /** 属性情報獲得対象マスク  */
#define VFS_VSTAT_MASK_GETATTR						\
	( VFS_VSTAT_MASK_VNID   |					\
	VFS_VSTAT_MASK_DEV      |					\
	VFS_VSTAT_MASK_MODE_FMT |					\
	VFS_VSTAT_MASK_MODE_ACS |					\
	VFS_VSTAT_MASK_NLINK    |					\
	VFS_VSTAT_MASK_UID      |					\
        VFS_VSTAT_MASK_GID      |					\
        VFS_VSTAT_MASK_RDEV     |					\
	VFS_VSTAT_MASK_SIZE     |					\
	VFS_VSTAT_MASK_BLKSIZE  |					\
	VFS_VSTAT_MASK_NRBLKS   |					\
	VFS_VSTAT_MASK_ATIME    |					\
	VFS_VSTAT_MASK_MTIME    |					\
	VFS_VSTAT_MASK_CTIME )

/** デバイスのメジャー番号のシフト値 */
#define VFS_VSTAT_DEV_MAJOR_SHIFT		UINT64_C(32)
/** デバイスのメジャー番号のマスク値 */
#define VFS_VSTAT_DEV_MAJOR_MASK		( ( UINT64_C(1) << 32 ) - 1 )
/** デバイスのマイナー番号のシフト値 */
#define VFS_VSTAT_DEV_MINOR_SHIFT		UINT64_C(0)
/** デバイスのマイナー番号のマスク値 */
#define VFS_VSTAT_DEV_MINOR_MASK		( ( UINT64_C(1) << 32 ) - 1 )

/**
   デバイスのメジャー番号を得る
   @param[in] _dev デバイス番号
   @return デバイスのメジャー番号
*/
#define VFS_VSTAT_GET_MAJOR(_dev)					\
	((fs_dev_id)( ( ((dev_id)(_dev)) >> VFS_VSTAT_DEV_MAJOR_SHIFT ) & \
	    VFS_VSTAT_DEV_MAJOR_MASK) )
/**
   デバイスのマイナー番号を得る
   @param[in] _dev デバイス番号
   @return デバイスのマイナー番号
  */
#define VFS_VSTAT_GET_MINOR(_dev)					\
	((fs_dev_id)( ( ((dev_id)(_dev)) >> VFS_VSTAT_DEV_MINOR_SHIFT ) & \
	    VFS_VSTAT_DEV_MINOR_MASK) )
/**
   デバイス番号を得る
   @param[in] _major メジャー番号
   @param[in] _minor マイナー番号
   @return デバイス番号
  */
#define VFS_VSTAT_MAKEDEV(_major, _minor)				\
	( (dev_id)( ( ( ( ((dev_id)(_major)) & VFS_VSTAT_DEV_MAJOR_MASK ) \
		    << VFS_VSTAT_DEV_MAJOR_SHIFT ) ) |			\
	    ( ( ( ((dev_id)(_minor)) & VFS_VSTAT_DEV_MINOR_MASK )	\
		<< VFS_VSTAT_DEV_MINOR_SHIFT ) ) ) )
#endif  /* !ASM_FILE  */
#endif  /* _FS_VFS_VFS_ATTR_H  */
