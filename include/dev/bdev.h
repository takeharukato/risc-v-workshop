/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Block device definitions                                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_DEV_BDEV_H)
#define _DEV_BDEV_H

#include <kern/page-macros.h>
#include <klib/misc.h>

#define BDEV_MIN_SECSIZ        (512)           /**< 最小セクタ長(512バイト)              */
#define BDEV_DEFAULT_SECSIZ    BDEV_MIN_SECSIZ /**< デフォルトセクタ長(512バイト)        */
#define BDEV_DEFAULT_BLKSIZ    (PAGE_SIZE)     /**< デフォルトブロック長(ページサイズ)   */
#define BDEV_DEFAULT_CAPACITY  (~(INT64_C(0))) /**< デフォルトキャパシティ(ページサイズ) */

#if !defined(ASM_FILE)

#include <klib/freestanding.h>
#include <kern/kern-types.h>
#include <kern/spinlock.h>
#include <kern/wqueue.h>

#include <klib/align.h>
#include <klib/list.h>
#include <klib/queue.h>
#include <klib/rbtree.h>
#include <klib/refcount.h>

#include <kern/vfs-if.h>

typedef void *       bdev_private;  /**< デバイス固有情報       */

struct _bio_request;
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
	/** デバイスのセクタ長(単位:バイト) */
	size_t                                bdent_secsiz;
	/** デバイスのブロック長(単位:バイト) */
	size_t                                bdent_blksiz;
	/** デバイスの容量(単位:バイト) */
	size_t                              bdent_capacity;
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

/**
   セクタサイズが最小セクタサイズ(512バイト)の等倍であることを確認
   @param[in] _secsiz セクタサイズ
   @retval    真 ブロックサイズが最小セクタサイズの等倍で, かつ, 最小セクタサイズ以上である
   @retval    偽 ブロックサイズが最小セクタサイズの等倍でないか, 最小セクタサイズより小さい
 */
#define BDEV_IS_SECTORSIZE_VALID(_secsiz)	\
	( ( !addr_not_aligned((_secsiz), BDEV_MIN_SECSIZ) ) \
	    && ( (_secsiz) >= (BDEV_MIN_SECSIZ) ) )

/**
   ブロックサイズがセクタサイズの等倍で, セクタサイズより大きいことを確認
   @param[in] _secsiz セクタサイズ
   @param[in] _blksiz ブロックサイズ
   @retval    真 ブロックサイズがセクタサイズの等倍で, かつ, セクタサイズ以上である
   @retval    偽 ブロックサイズがセクタサイズの等倍でないか, セクタサイズより小さい
 */
#define BDEV_IS_BLOCKSIZE_VALID(_secsiz, _blksiz)	\
	( ( !addr_not_aligned((_blksiz), (_secsiz)) ) && ( (_blksiz) >= (_secsiz) ) )

int bdev_page_cache_pool_set(struct _bdev_entry *_bdev, struct _vfs_page_cache_pool *_pool);

int bdev_blocksize_set(dev_id _devid, size_t _blksiz);
int bdev_blocksize_get(dev_id _devid, size_t *_blksizp);
int bdev_sectorsize_set(dev_id _devid, size_t _secsiz);
int bdev_sectorsize_get(dev_id _devid, size_t *_secsizp);
int bdev_capacity_set(dev_id _devid, size_t _capacity);
int bdev_capacity_get(dev_id _devid, size_t *_capacityp);

int bdev_page_cache_get(dev_id _devid, off_t _offset, struct _vfs_page_cache **_pcachep);

int bdev_bdev_entry_get(dev_id _devid, struct _bdev_entry **_bdevp);
int bdev_bdev_entry_put(struct _bdev_entry *_bdev);

int bdev_add_request(dev_id _devid, struct _bio_request *_req);
int bdev_handle_requests(dev_id _devid);

int bdev_device_register(dev_id _devid, struct _fs_calls *_ops, bdev_private _private);
void bdev_device_unregister(dev_id _devid);

void bdev_init(void);
void bdev_finalize(void);
#endif  /*  !ASM_FILE  */
#endif  /*  _DEV_BDEV_H  */
