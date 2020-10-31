/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Block device definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_BLOCK_DEVICE_H)
#define _DEV_BLOCK_DEVICE_H

/*
 * I/Oの向き
 */
#define BIO_DIR_READ       (0)  /**< デバイスからの読み込み */
#define BIO_DIR_WRITE      (1)  /**< デバイスへの書き込み   */

/*
 * I/Oリクエストの状態
 */
#define BIO_STATE_NONE     (0)  /**< 初期状態  */
#define BIO_STATE_BUSY     (1)  /**< I/O処理中 */
#define BIO_STATE_FINISHED (2)  /**< I/O完了   */
#define BIO_STATE_ERROR    (4)  /**< エラー    */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <kern/wqueue.h>

#include <klib/list.h>
#include <klib/queue.h>
#include <klib/rbtree.h>
#include <klib/refcount.h>

#include <kern/vfs-if.h>

typedef void *       bdev_private;  /**< デバイス固有情報       */

struct _vfs_page_cache_pool;
struct _bdev_entry;

/**
   ブロックデバイスエントリ
*/
typedef struct _bdev_entry{
	spinlock                                bdent_lock; /**< ロック                    */
	struct _refcounter                      bdent_refs; /**< 参照カウンタ              */
	RB_ENTRY(_bdev_entry)                    bdent_ent; /**< ブロックデバイスエントリ  */
	dev_id                                 bdent_devid; /**< デバイスID                */
	/** デバイスのブロック長(単位:バイト) */
	size_t                                bdent_blksiz;
	struct _queue                           bdent_rque; /**< リクエストキュー          */
	struct _fs_calls                       bdent_calls; /**< ファイル操作IF            */
	struct _vfs_page_cache_pool            *bdent_pool; /**< ページキャッシュプール    */
	bdev_private                         bdent_private; /**< デバイス固有情報          */
}bdev_entry;

/**
   ブロックデバイスDB構造体
*/
typedef struct _bdev_db{
	spinlock                         bdev_lock;  /**< ロック                   */
	RB_HEAD(_bdev_tree, _bdev_entry) bdev_head;  /**< ブロックデバイスDB       */
}bdev_db;

/**
   ブロックデバイスDB初期化子
   @param[in] _bdevdbp ブロックデバイスDBのポインタ
 */
#define __BDEVDB_INITIALIZER(_bdevdbp) {		                            \
	.bdev_lock = __SPINLOCK_INITIALIZER,		                            \
	.bdev_head  = RB_INITIALIZER(&((_bdevdbp)->bdev_head)),		            \
	}
int bdev_page_cache_pool_set(struct _bdev_entry *_bdev, struct _vfs_page_cache_pool *_pool);

int bdev_block_size_set(dev_id _devid, size_t _blksiz);
int bdev_block_size_get(dev_id _devid, size_t *_blksizp);
int bdev_page_cache_get(dev_id _devid, off_t _offset, struct _vfs_page_cache **_pcachep);

int bdev_bdev_entry_get(dev_id _devid, struct _bdev_entry **_bdevp);
int bdev_bdev_entry_put(struct _bdev_entry *_bdev);

int bdev_device_register(dev_id _devid, size_t _blksiz, struct _fs_calls *_ops,
    bdev_private _private);
void bdev_device_unregister(dev_id _devid);

void bdev_init(void);
void bdev_finalize(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _DEV_BLOCK_DEVICE_H  */
