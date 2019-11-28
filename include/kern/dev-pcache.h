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

#include <klib/splay.h>
#include <klib/refcount.h>

#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/wqueue.h>

#define PCACHE_STATE_BUSY             (0x1)  /**< ページキャッシュがロックされている       */
#define PCACHE_STATE_VALID            (0x2)  /**< ディスクの内容を読み込み済み             */
#define PCACHE_STATE_DIRTY            (0x4)  /**< ページキャッシュの方がディスクより新しい */

/**
   ページキャッシュ
 */
typedef struct _page_cache{
	refcounter                     refs;  /**< 参照カウンタ                         */
	struct _mutex                   mtx;  /**< 排他用ロック(状態更新用)             */
	struct _wque_waitqueue      waiters;  /**< ページバッファ待ちキュー             */
	SPLAY_ENTRY(_page_cache)        ent;  /**< SPLAY木へのエントリ                  */
	dev_id                       bdevid;  /**< デバイスID                           */
	off_t                        offset;  /**< オフセット (単位:バイト)             */
	pcache_state                  state;  /**< バッファの状態                       */
	void                       *pb_data;  /**< ページバッファデータへのポインタ     */
}page_cache;

/**
   ページキャッシュプール
 */
typedef struct _page_cache_pool{
	spinlock                              lock; /**< ページキャッシュツリーのロック */
	size_t                               pgsiz; /**< ページサイズ                   */
	SPLAY_HEAD(_pcache_tree, _page_cache) head; /**< ページキャッシュツリー         */
}page_cache_pool;

#endif  /*  _KERN_DEV_PCACHE_H  */
