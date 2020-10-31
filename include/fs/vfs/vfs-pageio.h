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
#define	VFS_PCACHE_INVALID           (0x0)    /**< 無効なキャッシュ                         */
#define	VFS_PCACHE_BUSY              (0x1)    /**< ページキャッシュがロックされている       */
#define	VFS_PCACHE_CLEAN             (0x2)    /**< ディスクとキャッシュの内容が一致している */
#define	VFS_PCACHE_DIRTY             (0x4)    /**< ページキャッシュの方がディスクより新しい */
/** 更新可能なページキャッシュ状態 */
#define VFS_PCACHE_STATE_MASK			\
	(VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY)
/*
 * ページキャッシュプールの状態
 */
#define	PCPOOL_DORMANT           (0x0)    /**< 無効なページキャッシュプール             */
#define	PCPOOL_CREATED           (0x1)    /**< 有効なページキャッシュプール             */
#define	PCPOOL_DELETE            (0x2)    /**< 削除予約されている                       */

typedef uint32_t vfs_pcache_pool_state; /**< ページキャッシュプールの状態 */

struct _bdev_entry;
struct _block_buffer;

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
	/** オフセットアドレス (単位:バイト, ページアライン)     */
	off_t                          pc_offset;
	/** LRUリストのエントリ                  */
	struct _list                 pc_lru_link;
	/** ブロックバッファキュー               */
	struct _queue                 pc_buf_que;
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
	/** ブロックデバイス           */
	struct _bdev_entry                             *pcp_bdev;
	/** ファイルのv-node           */
	struct _vnode                                 *pcp_vnode;
	/**  ページサイズ(単位:バイト) */
	size_t                                         pcp_pgsiz;
	/**  ページキャッシュツリー    */
	RB_HEAD(_vfs_pcache_tree, _vfs_page_cache)     pcp_head;
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

/**
   ページキャッシュが利用中であることを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_BUSY(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_BUSY )

/**
   ページキャッシュと2時記憶の状態が一致していることを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_CLEAN(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_CLEAN )

/**
   ページキャッシュの方が2時記憶より新しいことを確認する
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_DIRTY(_pcache) \
	( (_pcache)->pc_state & VFS_PCACHE_DIRTY )

/**
   ページキャッシュが有効であることを確認する
   ページキャッシュと2次記憶の内容が一致しているか,
   キャッシュのほうが新しい場合のいずれかの状態にある場合に
   有効とする
   @param[in] _pcache ページキャッシュ
 */
#define VFS_PCACHE_IS_VALID(_pcache)					\
	( ( (_pcache)->pc_state & ( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) \
	    && ( ( (_pcache)->pc_state & \
		    ( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) != \
		( VFS_PCACHE_CLEAN | VFS_PCACHE_DIRTY ) ) )

bool vfs_page_cache_ref_inc(struct _vfs_page_cache *_pc);
bool vfs_page_cache_ref_dec(struct _vfs_page_cache *_pc);
int vfs_page_cache_mark_clean(struct _vfs_page_cache *_pc);
int vfs_page_cache_mark_dirty(struct _vfs_page_cache *_pc);
int vfs_page_cache_pagesize_get(struct _vfs_page_cache *_pc, size_t *_sizep);
int vfs_page_cache_enqueue_block_buffer(struct _vfs_page_cache *_pc,
    struct _block_buffer *_buf);
int vfs_page_cache_dequeue_block_buffer(struct _vfs_page_cache *_pc,
    struct _block_buffer **_bufp);
bool vfs_page_cache_pool_ref_inc(struct _vfs_page_cache_pool *_pool);
bool vfs_page_cache_pool_ref_dec(struct _vfs_page_cache_pool *_pool);

int vfs_page_cache_get(struct _vfs_page_cache_pool *_pool, off_t _offset,
    struct _vfs_page_cache **_pcp);
int vfs_page_cache_put(struct _vfs_page_cache *_pc);

void vfs_init_pageio(void);
void vfs_finalize_pageio(void);
#endif  /* !ASM_FILE */
#endif  /*  _FS_VFS_VFS_PAGEIO_H  */
