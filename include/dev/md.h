/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Memory block device definitions                                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_MD_H)
#define _DEV_MD_H

#include <kern/page-macros.h>
#include <klib/misc.h>

#if 0 /* TODO: ページプール作成時に有効化 */
#define MD_SECTOR_SIZE         BDEV_MIN_SECSIZ    /**< 最小セクタ長(512バイト) */
#else
#define MD_SECTOR_SIZE         (512)           /**< 最小セクタ長(512バイト) */
#endif

#define MD_BLOCK_SIZE          PAGE_SIZE       /**< ページサイズ            */

#define MD_DEV_MAJOR_NR        (2)             /**< メジャー番号            */
#define MD_DEV_MINOR_MAX       (8)             /**< 最大デバイス数          */

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <kern/spinlock.h>
#include <kern/vfs-if.h>

/**
   メモリブロックデバイス固有情報
 */
typedef struct _md_device_info{
	spinlock                       md_lock; /**< ロック変数   */
	dev_fsid                      md_major; /**< メジャー番号 */
	dev_fsid                      md_minor; /**< マイナー番号 */
	size_t                       md_secsiz; /** セクタ長(単位:バイト) */
	size_t                       md_blksiz; /** ブロック長(単位:バイト) */
	size_t                     md_capacity; /** 容量(単位:バイト) */
	fs_calls                     *md_calls; /** デバイスドライバIF */
}md_device_info;

int md_init(void);
void md_finalize(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _DEV_MD_H  */
