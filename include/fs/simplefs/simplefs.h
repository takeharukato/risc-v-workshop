/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  simple file system definitions                                    */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_SIMPLEFS_SIMPLEFS_H)
#define _FS_SIMPLEFS_SIMPLEFS_H

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <klib/bitops.h>
#include <kern/kern-types.h>
#include <kern/mutex.h>

#define SIMPLEFS_SUPER_NR    (8)    /**< 8マウント    */
#define SIMPLEFS_INODE_NR    (64)   /**< 64ファイル   */
#define SIMPLEFS_IDATA_NR    (128)  /**< 512 KiB      */
#define SIMPLEFS_DIRSIZ      (60)   /**< ファイル名長 */

typedef struct _simplefs_data{
	uint64_t data[512];  /**< 1ページ分 */
}simplefs_data;

typedef struct _simplefs_inode{
	uint16_t                     i_mode;  /**< ファイル種別/保護属性            */
	uint16_t                   i_nlinks;  /**< ファイルのリンク数 (単位:個)     */
	uint16_t                      i_uid;  /**< ファイル所有者のユーザID         */
	uint16_t                      i_gid;  /**< ファイル所有者のグループID       */
	uint32_t                     i_size;  /**< ファイルサイズ (単位:バイト)     */
	uint32_t                    i_atime;  /**< 最終アクセス時刻 (単位:UNIX時刻) */
	uint32_t                    i_mtime;  /**< 最終更新時刻 (単位:UNIX時刻)     */
	uint32_t                    i_ctime;  /**< 最終属性更新時刻 (単位:UNIX時刻) */
	struct _simplefs_data        i_data[SIMPLEFS_IDATA_NR];   /* データブロック */
}simplefs_inode;

typedef struct _simplefs_dent{
	uint32_t                d_inode;  /**< I-node番号                                  */
	char    d_name[SIMPLEFS_DIRSIZ];  /**< ファイル名 (NULLターミネートなし, 60バイト) */
}simplefs_dent;

/**
   スーパーブロック情報
 */
typedef struct _simplefs_super_block{
	struct _simplefs_inode s_inode[SIMPLEFS_INODE_NR];  /* ファイル */
}simplefs_super_block;
/**
   ファイルシステム管理用大域データ
 */
typedef struct _simplefs_table{
	struct _mutex     mtx;  /**< 排他用mutex */
	
}simplefs_table;
#endif  /*  !ASM_FILE  */
#endif  /*  _FS_SIMPLEFS_SIMPLEFS_H   */
