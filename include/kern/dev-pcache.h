/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Cache definition                                             */
/*                                                                    */
/**********************************************************************/
#if !defined(_KERN_DEV_PCACHE_H)
#define  _KERN_DEV_PCACHE_H

#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/queue.h>
#include <klib/splay.h>
#include <klib/refcount.h>

#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/wqueue.h>
#include <kern/page-if.h>
/*
 * ページキャッシュの状態
 */
#define PCACHE_STATE_INVALID          (0x0)  /**< 無効なキャッシュ                         */
#define PCACHE_STATE_BUSY             (0x1)  /**< ページキャッシュがロックされている       */
#define PCACHE_STATE_CLEAN            (0x2)  /**< ディスクとキャッシュの内容が一致している */
#define PCACHE_STATE_DIRTY            (0x4)  /**< ページキャッシュの方がディスクより新しい */

/**
   ページキャッシュ
   @note BUSY以外のページキャッシュの状態更新はページキャッシュをBUSYの状態に遷移した後
   (他スレッドがページの更新権を得ないことを保証した後)、ページキャッシュプールのロックを
   解放してから、ページのmtxを獲得して行う。
   I/Oウエイトが発生するので, ページキャッシュプールのロックを獲得したままI/O操作を
   行わないようにしつつ、ページの内容更新を排他的に実施するためである。
 */
typedef struct _page_cache{
	struct _mutex                   mtx;  /**< 排他用ロック(状態更新用)             */
	struct _wque_waitqueue      waiters;  /**< ページバッファ待ちキュー             */
	SPLAY_ENTRY(_page_cache)        ent;  /**< SPLAY木へのエントリ                  */
	refcounter                     refs;  /**< 参照カウンタ                         */
	dev_id                       bdevid;  /**< デバイスID                           */
	size_t                        pgsiz;  /**< ページサイズ                   */
	off_t                        offset;  /**< オフセット (単位:バイト)             */
	pcache_state                  state;  /**< バッファの状態                       */
	struct _page_frame              *pf;  /**< ページフレーム情報                   */
	void                       *pc_data;  /**< ページキャッシュデータへのポインタ   */
}page_cache;

/**
   ページキャッシュプール
   @note ページキャッシュプールのロックは各ページキャッシュのBUSY状態の更新と
         ページキャッシュプールのSPLAY木の更新を同時に排他的に行うために使用する。
	 各ページキャッシュのBUSY状態の更新はかならずページキャッシュプールのロックを
	 獲得して行う。
 */
typedef struct _page_cache_pool{
	spinlock                              lock; /**< ページキャッシュツリーのロック */
	size_t                               pgsiz; /**< ページサイズ                   */
	SPLAY_HEAD(_pcache_tree, _page_cache) head; /**< ページキャッシュツリー         */
	struct _queue                          lru; /**< LRUキャッシュ                  */
}page_cache_pool;

/**
   ページキャッシュプールの初期化子
   @param[in] _pcache_pool  ページキャッシュプールのアドレス
   @param[in] _pgsiz        プール内のページサイズ
 */
#define __PCACHE_POOL_INITIALIZER(_pcache_pool, _pgsiz)	        \
	{.lock = __SPINLOCK_INITIALIZER,                        \
	.pgsiz = (_pgsiz),                                      \
	.head = SPLAY_INITIALIZER(&((_pcache_pool)->head)),	\
	.lru  = __QUEUE_INITIALIZER(&((_pcache_pool)->lru)),    \
	}

/**
   ページキャッシュが利用中であることを確認する
   @param[in] _pcache プール内のページサイズ
 */
#define PCACHE_IS_BUSY(_pcache) \
	( (_pcache)->state & PCACHE_STATE_BUSY )

/**
   ページキャッシュと2時記憶の状態が一致していることを確認する
   @param[in] _pcache プール内のページサイズ
 */
#define PCACHE_IS_CLEAN(_pcache) \
	( (_pcache)->state & PCACHE_STATE_CLEAN )

/**
   ページキャッシュの方が2時記憶より新しいことを確認する
   @param[in] _pcache プール内のページサイズ
 */
#define PCACHE_IS_DIRTY(_pcache) \
	( (_pcache)->state & PCACHE_STATE_DIRTY )

/**
   ページキャッシュが有効であることを確認する
   ページキャッシュと2次記憶の内容が一致しているか,
   キャッシュのほうが新しい場合のいずれかの状態にある場合に
   有効とする
   @param[in] _pcache プール内のページサイズ
 */
#define PCACHE_IS_VALID(_pcache)					\
	( ( (_pcache)->state & ( PCACHE_STATE_CLEAN | PCACHE_STATE_DIRTY ) ) &&	\
	    ( ( (_pcache)->state & ( PCACHE_STATE_CLEAN | PCACHE_STATE_DIRTY ) ) != \
		( PCACHE_STATE_CLEAN | PCACHE_STATE_DIRTY ) ) )

int pagecache_pagesize(dev_id dev, size_t *pgsizp);
int pagecache_get(dev_id dev, off_t offset, struct _page_cache **pcp);
void pagecache_put(struct _page_cache *_pc);
void pagecache_mark_dirty(struct _page_cache *_pc);
int pagecache_read(dev_id dev, off_t offset, struct _page_cache **pcp);
void pagecache_write(struct _page_cache *_pc);
void pagecache_shrink_pages(obj_cnt_type _max, bool wbonly, obj_cnt_type *_free_nrp);
void pagecache_init(void);
#endif  /*  _KERN_DEV_PCACHE_H  */
