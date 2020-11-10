/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system page cache pool definitions                   */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_PCPOOL_H)
#define  _FS_VFS_VFS_PCPOOL_H

#if !defined(ASM_FILE)
#include <klib/freestanding.h>
#include <kern/kern-types.h>

#include <klib/refcount.h>
#include <klib/list.h>
#include <klib/rbtree.h>

#include <kern/wqueue.h>
#include <kern/spinlock.h>
#include <kern/mutex.h>

#include <fs/vfs/vfs-types.h>
#include <fs/vfs/vfs-attr.h>

/*
 * ページキャッシュプールの状態
 */
#define	PCPOOL_DORMANT           (0x0)    /**< 無効なページキャッシュプール             */
#define	PCPOOL_CREATED           (0x1)    /**< 有効なページキャッシュプール             */
#define	PCPOOL_DELETE            (0x2)    /**< 削除予約されている                       */

/*
 * ページ回収指示
 */
#define PCPOOL_INVALIDATE_ALL    (-1)     /**< 全ページキャッシュを無効化する */

typedef uint32_t vfs_pcache_pool_state;   /**< ページキャッシュプールの状態 */

struct _vfs_page_cache;

/**
   ページキャッシュプール
 */
typedef struct _vfs_page_cache_pool{
	/** 排他用ロック(ページキャッシュの使用権/キュー更新用) */
	struct _mutex                                    pcp_mtx;
	/** 参照カウンタ               */
	refcounter                                      pcp_refs;
	/** ページキャッシュプールの状態 */
        vfs_pcache_pool_state                          pcp_state;
	/** ブロックデバイスのデバイスID */
	dev_id                                        pcp_bdevid;
	/**  ページサイズ(単位:バイト) */
	size_t                                         pcp_pgsiz;
	/**  ページキャッシュツリー    */
	RB_HEAD(_vfs_pcache_tree, _vfs_page_cache)      pcp_head;
	/**  LRUキャッシュ (二次記憶との一貫性がとれているページ)     */
	struct _queue                              pcp_clean_lru;
	/**  LRUキャッシュ (二次記憶よりキャッシュの方が新しいページ) */
	struct _queue                              pcp_dirty_lru;
}vfs_page_cache_pool;

/**
   ページキャッシュプールDB
 */
typedef struct _vfs_page_cache_pool_db{
	/** ページキャッシュプールDBのロック */
	spinlock                         lock;
	/** ページキャッシュプールDB         */
	RB_HEAD(_vfs_page_cache_pool_tree, _vfs_page_cache_pool) head;
}vfs_page_cache_pool_db;

bool vfs_page_cache_pool_ref_inc(struct _vfs_page_cache_pool *_pool);
bool vfs_page_cache_pool_ref_dec(struct _vfs_page_cache_pool *_pool);
int vfs_page_cache_pool_shrink(struct _vfs_page_cache_pool *_pool,
    singned_cnt_type _reclaim_nr, singned_cnt_type *_reclaimedp);
int vfs_page_cache_invalidate(struct _vfs_page_cache *_pc);
int vfs_page_cache_pool_pagesize_get(struct _vfs_page_cache_pool *_pool, size_t *_pagesizep);
int vfs_page_cache_get(struct _vfs_page_cache_pool *_pool, off_t _offset,
    struct _vfs_page_cache **_pcp);
void vfs_init_page_cache_pool(void);
void vfs_finalize_page_cache_pool(void);
#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_PCPOOL_H  */
