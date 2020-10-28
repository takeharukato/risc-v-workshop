/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Virtual file system page I/O definitions                          */
/*                                                                    */
/**********************************************************************/
#if !defined(_FS_VFS_VFS_PAGEIO_H)
#define  _FS_VFS_VFS_PAGEIO_H

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

/*
 * ページキャッシュの状態
 */
#define	PCACHE_INVALID           (0x0)    /**< 無効なキャッシュ                         */
#define	PCACHE_BUSY              (0x1)    /**< ページキャッシュがロックされている       */
#define	PCACHE_CLEAN             (0x2)    /**< ディスクとキャッシュの内容が一致している */
#define	PCACHE_DIRTY             (0x4)    /**< ページキャッシュの方がディスクより新しい */

/*
 * ページキャッシュプールの状態
 */
#define	PCPOOL_DORMANT           (0x0)    /**< 無効なページキャッシュプール             */
#define	PCPOOL_CREATED           (0x1)    /**< 有効なページキャッシュプール             */
#define	PCPOOL_DELETE            (0x2)    /**< 削除予約されている                       */

typedef uint32_t vfs_pcache_pool_state; /**< ページキャッシュプールの状態 */

struct _bdev_entry;
/**
   ページキャッシュ
 */
typedef struct _vfs_page_cache{
	/** ページキャッシュデータ構造更新ロック */
	spinlock                         pc_lock;
	/** 排他用ロック(状態/ページ更新用)      */
	struct _mutex                     pc_mtx;
	/** 参照カウンタ                         */
	refcounter                       pc_refs;
	/** ページキャッシュプールへのリンク     */
	struct _vfs_page_cache_pool  *pc_pcplink;
	/** バッファの状態                       */
	pcache_state                    pc_state;
	/** パディング                           */
	uint32_t                            pad1;
	/** ページバッファ待ちキュー             */
	struct _wque_waitqueue        pc_waiters;
	/** ファイル/ブロックデバイス中のオフセットをキーとした検索用RB木エントリ        */
	RB_ENTRY(_vfs_page_cache)         pc_ent;
	/** オフセットアドレス (単位:バイト)     */
	off_t                          pc_offset;
	/** LRUリストのエントリ                  */
	struct _list                 pc_lru_link;
	/** ページフレーム情報                   */
	struct _page_frame                *pc_pf;
	/** ページキャッシュデータへのポインタ   */
	void                            *pc_data;
}vfs_page_cache;

/**
   ページキャッシュプール
 */
typedef struct _vfs_page_cache_pool{
	/** 排他用ロック(キュー更新用) */
	struct _mutex                                    pcp_mtx;
	/** 参照カウンタ               */
	refcounter                                      pcp_refs;
	/** ページキャッシュプールの状態 */
        vfs_pcache_pool_state                          pcp_state;
	/** ブロックデバイス         */
	struct _bdev_entry                             *pcp_bdev;
	/** ファイルのv-node           */
	struct _vnode                                 *pcp_vnode;
	/**  ページサイズ              */
	size_t                                         pcp_pgsiz;
	/**  ページキャッシュツリー    */
	RB_HEAD(_pcache_bdev_tree, _vfs_page_cache)     pcp_head;
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
	RB_HEAD(_page_cache_pool_tree, _page_cache_pool) head;
}vfs_page_cache_pool_db;

/**
   ページキャッシュプールDB初期化子
   @param[in] _pcpdb ページキャッシュプールDBのアドレス
 */
#define __PCPDB_INITIALIZER(_pcpdb) {		                            \
	.lock = __SPINLOCK_INITIALIZER,		                            \
	.head  = RB_INITIALIZER(&((_pcpdb)->head)),		            \
	}

bool vfs_page_cache_ref_inc(struct _vfs_page_cache *_pc);
bool vfs_page_cache_ref_dec(struct _vfs_page_cache *_pc);
bool vfs_page_cache_pool_ref_inc(struct _vfs_page_cache_pool *_pool);
bool vfs_page_cache_pool_ref_dec(struct _vfs_page_cache_pool *_pool);
int vfs_dev_page_cache_alloc(struct _bdev_entry *_bdev);
void vfs_init_pageio(void);
void vfs_finalize_pageio(void);
#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_PAGEIO_H  */
