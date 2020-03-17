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

struct _vnode;

/** ファイル属性
 */
typedef struct _file_stat {
	vfs_vnode_id   st_vnid; /**< vnode id                                */
	dev_id          st_dev; /**< ファイルがあるデバイスの ID             */
	vfs_fs_mode    st_mode; /**< ファイル種別/アクセス権                 */
	vfs_fs_nlink  st_nlink; /**< リンク数                                */  
	cred_id         st_uid; /**< ユーザID                                */
	cred_id         st_gid; /**< グループID                              */
	dev_id         st_rdev; /**< デバイスドライバのデバイスID            */
	off_t          st_size; /**< ファイルサイズ                          */
	off_t       st_blksize; /**< ファイルシステム I/O でのブロックサイズ */
	obj_cnt_type st_blocks; /**< 割り当てられたブロック数                */
	epoch_time    st_atime; /**< 最終アクセス時刻                        */
	epoch_time    st_mtime; /**< 最終修正時刻                            */
	epoch_time    st_ctime; /**< 最終状態変更時刻                        */
}file_stat;
/*
 * 操作対象メンバ
 */
#define VFS_VSTAT_MASK_NONE      (0x0000)  /**< 属性情報をコピーしない      */
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
#define VFS_VSTAT_MASK_ATIME     (0x0800)  /**< アクセス時刻                */
#define VFS_VSTAT_MASK_MTIME     (0x1000)  /**< 修正時刻                    */
#define VFS_VSTAT_MASK_CTIME     (0x2000)  /**< 最終状態変更時刻            */

#define VFS_VSTAT_UID_ROOT      (0)       /**< rootユーザID                 */
#define VFS_VSTAT_GID_ROOT      (0)       /**< rootグループID               */

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
	( ~( VFS_VSTAT_MASK_CHATTR   |					\
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

#endif  /* !ASM_FILE  */
#endif  /* _FS_VFS_VFS_ATTR_H  */
